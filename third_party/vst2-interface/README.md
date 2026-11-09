# VST2 Interface Definitions

Unmodified `include/steinberg/vst2.h` from the Linux Studio Plugins project's
[lsp-3rd-party repository](https://github.com/lsp-plugins/lsp-3rd-party/blob/37ce46ffac9e523cc2cfc22d39658b79ef154269/include/steinberg/vst2.h).

- Commit: `37ce46ffac9e523cc2cfc22d39658b79ef154269`
- SHA-256: `82f4628a625de06407307496ae1f30d826bc87986161f8a937e95f34d2b8b8b8`
- License: ISC, reproduced in the header. The project's Apache license does not replace this notice.

This replaces the incomplete handwritten ABI definitions, not the existing
VST2 SDK helper implementation. Binary-boundary tests intentionally do not
include this header so production and host tests cannot share layout mistakes.
