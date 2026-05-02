# poc-smollm2

Dissecting SmolLM2

## The implementation

Code:
* `build.c`      - builder

Code of how certain bits work:
* `vocab.c`      - tokenisation

No library usages, it should be buildable as long as you have C compiler.

The code is not for production. It is meant for short-lived processes, so just
leaks memory and resources. Also, it will not work fine if you define NDEBUG
because `assert`s are used for runtime logic.

You need these files:
* https://huggingface.co/HuggingFaceTB/SmolLM2-135M/blob/main/merges.txt
* https://huggingface.co/HuggingFaceTB/SmolLM2-135M/blob/main/model.safetensors
* https://huggingface.co/HuggingFaceTB/SmolLM2-135M/blob/main/vocab.json

Using `curl`, something like:

```bash
curl -L -o merges.txt https://huggingface.co/HuggingFaceTB/SmolLM2-135M/resolve/main/merges.txt
curl -L -o model.safetensors https://huggingface.co/HuggingFaceTB/SmolLM2-135M/resolve/main/model.safetensors?download=true
curl -L -o vocab.json https://huggingface.co/HuggingFaceTB/SmolLM2-135M/resolve/main/vocab.json?download=true
```

Compiling each C file is a simple `cc` invocation. But, if you have access to
`glslang` you can use `build.c` to automate the build:

```bash
clang -o build build.c
./build
```

