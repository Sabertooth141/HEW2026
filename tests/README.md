# Ore state tests

`OreStateTests.cpp` tests the public, rendering-independent `OreState` API with
standard C++ only. It does not need DirectX, ImGui, vcpkg, textures, or a game
window. Checks remain active when `NDEBUG` is defined; a failure returns exit
code `1` and prints its test name, line, and expression.

## Build and run

Open the **x64 Native Tools Command Prompt for VS 2026**, then run these commands
from the repository root:

```bat
if not exist work\tests mkdir work\tests
cl /nologo /std:c++20 /EHsc /W4 /utf-8 /O2 /DNDEBUG tests\OreStateTests.cpp /Fe:work\tests\OreStateTests.exe /Fo:work\tests\OreStateTests.obj
work\tests\OreStateTests.exe
```

The compiler and linker are supplied by the Visual Studio developer environment.
No dependency install or network access is involved. For a debug build, replace
`/O2 /DNDEBUG` with `/Od /Zi` and add `/Fd:work\tests\OreStateTests.pdb`.

## Coverage

- All three ore types initialize with their current provisional HP, collision
  damage, and experience values; custom stats remain configurable.
- Positive damage lowers HP without changing the maximum or reward stats.
- Exact lethal damage and overkill clamp HP to zero; later damage cannot revive
  or damage an already destroyed ore.
- Zero and negative damage, including `INT_MIN`, leave a live ore unchanged.
- `INT_MAX` HP and damage do not overflow or underflow.
- Invalid HP becomes one; negative collision damage and experience become zero.
- One swing cannot register twice during Stay or re-entry events, even if two
  hammers alternate contacts. A new swing is accepted, and the same swing can
  legitimately hit a different ore.
- ID zero is rejected; destroyed ore rejects all subsequent swing IDs.

The default-value expectations document temporary test balance. Update those
expectations if the planner-approved defaults change.

The state suite currently performs 201 checks. For the actual engine's collision,
damage, deferred scene removal, experience spawning/collection, enemy death,
scene reset, and original Game scene placement checks, build the game normally
and run `DXPractice.exe OreAcceptance` from `x64/Debug` or `x64/Release`.
The result is written to `ore-upload-acceptance.txt`; success returns exit code 0.
Use `DXPractice.exe OreDemo` for the interactive demonstration. The source ZIP
does not contain compiled executables.
