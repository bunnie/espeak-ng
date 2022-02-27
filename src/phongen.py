#!/usr/bin/python3

import argparse

def convert(ifile, ostruct):
    with open(ifile, "rb") as f:
        data = f.read()

        with open(ostruct + ".h", "w") as output:
            output.write("unsigned char {}[] = {{".format(ostruct + "_data"))
            nl = 0
            for d in data:
                if (nl % 32) == 0:
                    output.write('\n  ')
                nl += 1
                output.write('0x{:02x}, '.format(d))
            output.write("\n};\n")

def main():
    parser = argparse.ArgumentParser(description="Convert phoneme to C header file")
    parser.add_argument(
        "-f", "--file", required=True, help="filename to process", type=str
    )
    parser.add_argument(
        "-o", "--output-structure", required=False, help="name structure", type=str, default="phondata"
    )
    args = parser.parse_args()

    ifile = args.file

    convert(ifile, args.output_structure)


if __name__ == "__main__":
    main()

