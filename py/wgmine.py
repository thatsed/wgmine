import time
import argparse
import base64
from multiprocessing import Process
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric.x25519 import X25519PrivateKey


# benchmark cryptography: 110000K/s
# benchmark donna: 48000K/s


def mine(name):
    name = name.encode("ascii")
    
    while True:
        private = X25519PrivateKey.generate()
        public = private.public_key()
        b64 = base64.b64encode(
            public.public_bytes(
                serialization.Encoding.Raw, serialization.PublicFormat.Raw
            )
        )

        if b64.startswith(name):
            print(
                b64.decode("ascii"),
                base64.b64encode(
                    private.private_bytes(
                        serialization.Encoding.Raw,
                        serialization.PrivateFormat.Raw,
                        serialization.NoEncryption(),
                    )
                ).decode("ascii"),
            )



def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("name", type=str, help="name to search (start of pubkey)")
    parser.add_argument(
        "-m",
        "--multiprocessing",
        type=int,
        default=6,
        help="number of processes to spawn",
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_arguments()
    processes = []

    s = time.perf_counter()
    try:
        for i in range(args.multiprocessing):
            p = Process(target=mine, args=(args.name,))
            p.start()
            processes.append(p)

        p.join()
    except KeyboardInterrupt:
        pass
    finally:
        e = time.perf_counter()
        print(f"Elapsed: {e - s}s")
        for p in processes:
            p.kill()
