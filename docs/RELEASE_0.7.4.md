# CamelCrusher Rebuilt 0.7.4

In VST2, VST3, and AU, drag a knob up to increase its value or down to decrease it. The pointer now turns clockwise as the value increases. On Mix, fully left is dry and fully right is wet.

Only the controls changed. Sessions, presets, and automation keep their existing values and sound; there is no need to invert saved Mix settings. This release also includes the [0.7.3 stereo fixes](RELEASE_0.7.3.md).

Quit your audio host before installing. The installer offers VST2, VST3, and AU and, like the preceding release, is not notarized.

Built from source commit `35768498f8acc0ec1294128b671597e913dda66a`. All 21 Debug and Release tests passed, including offscreen checks of drag direction and pointer position. The extracted VST2 and VST3 editors were tested directly; the AU editor was tested in-process. The packaged VST2 produced the same measured output differences from dry as 0.7.3 at 0%, 25%, 50%, 75%, and 100% Mix. [Installer SHA-256](../downloads/CamelCrusher-Rebuilt-0.7.4.sha256).
