import httpx
import argparse
import faker

ARG_DEST = 'dest'
ARG_REQ = 'req'
ARG_VALID = 'valid'

ip_faker = faker.Faker()

def _ip_address_gen() -> str:
    return ip_faker.ipv4()

def _ip_address_gen_var(valid:bool) -> str:
    return '123.123.123.123' if valid else '111.111.111.111'

class HttpReqProducer:
    def __init__(self, cli_cfg:dict) -> None:
        self.dest = cli_cfg[ARG_DEST]
        self.request_no = int(cli_cfg[ARG_REQ])


    # add support for this
    def send_put_request(self) -> None:
        for _ in range(0, self.request_no):
            # make this as class
            header = {
                'x-forwarded-for': _ip_address_gen()
            }
            httpx.put(f'http://{self.dest}', headers=header, content="LoremIpsum")

    def send_get_request(self) -> None:
        for _ in range(0, self.request_no):
            # make this as class
            header = {
                'x-forwarded-for': _ip_address_gen()
            }
            httpx.get(f'http://{self.dest}', headers=header)

    def send_get_request_var(self, valid:bool) -> None:
        for _ in range(0, self.request_no):
            header = {
                'x-forwarded-for': _ip_address_gen_var(valid),
                'my_custom_hedaer': "foo_bar"
            }
            httpx.get(f'http://{self.dest}', headers=header)

def main() -> None:
    """
    Main function
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(f'--{ARG_DEST}', help='Destination', default='0.0.0.0:8000')
    parser.add_argument(f'--{ARG_REQ}', help='Number of HTTP request', default='1')
    parser.add_argument(f'--{ARG_VALID}', dest=ARG_VALID, help='use valid HTTP request', action='store_true')
    parser.add_argument("--invalid", dest=ARG_VALID, help='use invalid HTTP request', action='store_false')
    parser.set_defaults(ARG_VALID=True)

    cli_cfg = vars(parser.parse_args())
    http_gen = HttpReqProducer(cli_cfg)
    http_gen.send_get_request_var(cli_cfg[ARG_VALID])

if __name__ == "__main__":
    main()
