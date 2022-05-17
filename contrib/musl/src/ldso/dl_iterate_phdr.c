#include <elf.h>
#include <link.h>
#include "pthread_impl.h"
#include "libc.h"

#define AUX_CNT 38

extern weak hidden const size_t _DYNAMIC[];

static size_t _aux_v_phdr;
static size_t _aux_v_phnum;
static size_t _aux_v_phent;

void init_phdr(unsigned long phdr, unsigned long phnum, unsigned long phent) {
	_aux_v_phdr = phdr;
	_aux_v_phnum = phnum;
	_aux_v_phent = phent;
}

static int static_dl_iterate_phdr(int(*callback)(struct dl_phdr_info *info, size_t size, void *data), void *data)
{
	unsigned char *p;
	ElfW(Phdr) *phdr, *tls_phdr=0;
	size_t base = 0;
	size_t n;
	struct dl_phdr_info info;
	size_t i;

	for (p=(void *)_aux_v_phdr,n=_aux_v_phnum; n; n--,p+=_aux_v_phent) {
		phdr = (void *)p;
		if (phdr->p_type == PT_PHDR)
			base = _aux_v_phdr - phdr->p_vaddr;
		if (phdr->p_type == PT_DYNAMIC && _DYNAMIC)
			base = (size_t)_DYNAMIC - phdr->p_vaddr;
		if (phdr->p_type == PT_TLS)
			tls_phdr = phdr;
	}
	info.dlpi_addr  = base;
	info.dlpi_name  = "/proc/self/exe";
	info.dlpi_phdr  = (void *)_aux_v_phdr;
	info.dlpi_phnum = _aux_v_phnum;
	info.dlpi_adds  = 0;
	info.dlpi_subs  = 0;
	if (tls_phdr) {
		info.dlpi_tls_modid = 1;
		info.dlpi_tls_data = __tls_get_addr((tls_mod_off_t[]){1,0});
	} else {
		info.dlpi_tls_modid = 0;
		info.dlpi_tls_data = 0;
	}
	return (callback)(&info, sizeof (info), data);
}

weak_alias(static_dl_iterate_phdr, dl_iterate_phdr);
