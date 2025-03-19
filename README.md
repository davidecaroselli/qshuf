# qshuf - Quick Shuffle

`qshuf` is a fast and memory-efficient command-line tool for shuffling very large text files. It uses memory mapping to
minimize RAM usage, making it ideal for AI, machine learning, and data processing tasks that require randomized
datasets.

## Build from Source

To build `qshuf`, compile it using `cmake`:

```sh
mkdir build && cd build
cmake ..
make
```

### Examples

Shuffle a large file and print to stdout:

```sh
qshuf data.txt
```

Shuffle and save output to a file:

```sh
qshuf data.txt -o shuffled.txt
```

Use a specific random seed:

```sh
qshuf data.txt -s 42 > shuffled.txt
```

## License

This project is licensed under the MIT License. See `LICENSE` for details.

## Author

Created by Davide Caroselli. Contributions welcome!