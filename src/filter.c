#define _GNU_SOURCE

#include "filter.h"
#include "dbg.h"
#include "scopestdlib.h"
#include "yaml.h"


#define ALLOW_NODE          "allow"
#define ALLOW_PROCNAME_NODE   "procname"
#define ALLOW_ARG_NODE        "arg"
#define DENY_NODE           "deny"
#define DENY_PROCNAME_NODE    "procname"
#define DENY_ARG_NODE         "arg"

typedef enum {
    PROC_NOT_FOUND,
    PROC_ALLOWED,
    PROC_DENIED,
} proc_status;

typedef struct {
    const char *cmd;
    proc_status  status;
} filter_cfg_t;

typedef void (*node_filter_fn)(yaml_document_t *, yaml_node_t *, filter_cfg_t *);

typedef struct {
    yaml_node_type_t type;
    const char *key;
    node_filter_fn fn;
} parse_filter_table_t;

#define filter_foreach(pair, pairs) \
    for (pair = pairs.start; pair != pairs.top; pair++)

#define strScalarVal(node) ((const char*) node->data.scalar.value)

/*
* Process key value pair
*/
static void
processKeyValuePair(yaml_document_t *doc, yaml_node_pair_t *pair, const parse_filter_table_t *fEntry, filter_cfg_t *fCfg) {
    yaml_node_t* nodeKey = yaml_document_get_node(doc, pair->key);
    yaml_node_t* nodeValue = yaml_document_get_node(doc, pair->value);

    if (nodeKey->type != YAML_SCALAR_NODE) return;

    /*
    * Check if specific Node value is present and call proper function if exists
    */
    for (int i = 0; fEntry[i].type != YAML_NO_NODE; ++i) {
        if ((nodeValue->type == fEntry[i].type) &&
            (!scope_strcmp((char*)nodeKey->data.scalar.value, fEntry[i].key))) {
            fEntry[i].fn(doc, nodeValue, fCfg);
            break;
        }
    }
}

/*
* Process allow process name Scalar Node
*/
static void
processAllowProcNameScalar(yaml_document_t *doc, yaml_node_t *node, filter_cfg_t *fCfg) {
    if (node->type != YAML_SCALAR_NODE) return;

    if (!scope_strcmp(fCfg->cmd, strScalarVal(node))) {
        fCfg->status = PROC_ALLOWED;
    }
}

/*
* Process allow arg Scalar Node
*/
static void
processAllowArgScalar(yaml_document_t *doc, yaml_node_t *node, filter_cfg_t *fCfg) {
    if (node->type != YAML_SCALAR_NODE) return;

    if (scope_strstr(fCfg->cmd, strScalarVal(node)) != NULL) {
        fCfg->status = PROC_ALLOWED;
    }
}
/*
* Process allow sequence list
*/
static void
processAllowSeq(yaml_document_t *doc, yaml_node_t *node, filter_cfg_t *fCfg) {
    if (node->type != YAML_SEQUENCE_NODE) return;

    parse_filter_table_t t[] = {
        {YAML_SCALAR_NODE, ALLOW_PROCNAME_NODE, processAllowProcNameScalar},
        {YAML_SCALAR_NODE, ALLOW_ARG_NODE,      processAllowArgScalar},
        {YAML_NO_NODE,     NULL,                NULL}
    };

    yaml_node_item_t* seqItem;
    filter_foreach(seqItem, node->data.sequence.items) {
        yaml_node_t* nodeMap = yaml_document_get_node(doc, *seqItem);

        if (nodeMap->type != YAML_MAPPING_NODE) return;

        yaml_node_pair_t *pair;
        filter_foreach(pair, nodeMap->data.mapping.pairs) {
            processKeyValuePair(doc, pair, t, fCfg);
        }
    }
}

/*
* Process deny process name Scalar Node
*/
static void
processDenyProcNameScalar(yaml_document_t *doc, yaml_node_t *node, filter_cfg_t *fCfg) {
    if (node->type != YAML_SCALAR_NODE) return;

    if (!scope_strcmp(fCfg->cmd, strScalarVal(node))) {
        fCfg->status = PROC_DENIED;
    }
}

/*
* Process deny arg Scalar Node
*/
static void
processDenyArgScalar(yaml_document_t *doc, yaml_node_t *node, filter_cfg_t *fCfg) {
    if (node->type != YAML_SCALAR_NODE) return;

    if (scope_strstr(fCfg->cmd, strScalarVal(node)) != NULL) {
        fCfg->status = PROC_DENIED;
    }
}

/*
* Process deny sequence list
*/
static void
processDenySeq(yaml_document_t *doc, yaml_node_t *node, filter_cfg_t *fCfg) {
    if (node->type != YAML_SEQUENCE_NODE) return;

    parse_filter_table_t t[] = {
        {YAML_SCALAR_NODE, DENY_PROCNAME_NODE, processDenyProcNameScalar},
        {YAML_SCALAR_NODE, DENY_ARG_NODE,      processDenyArgScalar},
        {YAML_NO_NODE,     NULL,               NULL}
    };

    yaml_node_item_t* seqItem;
    filter_foreach(seqItem, node->data.sequence.items) {
        yaml_node_t* nodeMap = yaml_document_get_node(doc, *seqItem);

        if (nodeMap->type != YAML_MAPPING_NODE) return;

        yaml_node_pair_t *pair;
        filter_foreach(pair, nodeMap->data.mapping.pairs) {
            processKeyValuePair(doc, pair, t, fCfg);
        }
    }
}

/*
* Process Root node (starting point)
*/
static void
processRootNode(yaml_document_t *doc, filter_cfg_t *fCfg) {
    yaml_node_t *node = yaml_document_get_root_node(doc);

    if ((node == NULL) || (node->type != YAML_MAPPING_NODE)) {
        return;
    }

    parse_filter_table_t t[] = {
        {YAML_SEQUENCE_NODE, ALLOW_NODE, processAllowSeq},
        {YAML_SEQUENCE_NODE, DENY_NODE,  processDenySeq},
        {YAML_NO_NODE,       NULL,       NULL}
    };

    yaml_node_pair_t *pair;
    filter_foreach(pair, node->data.mapping.pairs) {
        processKeyValuePair(doc, pair, t, fCfg);
    }
    return;
}

/*
* Parse scope filter file
 *
 * Returns TRUE if filter file was successfully parsed, FALSE otherwise
 */
static bool
filterParseFile(const char* filterPath, filter_cfg_t *fCfg) {
    FILE *fs;
    bool status = FALSE;
    yaml_parser_t parser;
    yaml_document_t doc;

    if ((fs = scope_fopen(filterPath, "rb")) == NULL) {
        return status;
    }

    int res = yaml_parser_initialize(&parser);
    if (!res) {
        goto cleanup_filter_file;
    }

    yaml_parser_set_input_file(&parser, fs);

    res = yaml_parser_load(&parser, &doc);
    if (!res) {
        goto cleanup_parser;
    }

    processRootNode(&doc, fCfg);

    status = TRUE;

    yaml_document_delete(&doc);

cleanup_parser:
    yaml_parser_delete(&parser);

cleanup_filter_file:
    scope_fclose(fs);

    return status;
}

/*
* Verify against filter file if specifc process command should be scoped
 *
 * Returns TRUE if cmd will be scoped, FALSE otherwise
 */
bool
filterVerifyProc(const char *cmd, const char *filterPath) {

    if (cmd == NULL) {
        DBG(NULL);
        return FALSE;
    }

    /*
    * Default behavior: if filter file do not exists scope it
    */

    if (!filterPath || scope_access(filterPath, R_OK)) {
        return TRUE;
    }

    filter_cfg_t fCfg = {.cmd = cmd, .status = PROC_NOT_FOUND};

    bool res = filterParseFile(filterPath, &fCfg);
    return (res == TRUE) ?  (fCfg.status == PROC_ALLOWED) : FALSE;
}
