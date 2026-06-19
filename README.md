# X-DOS v0.1

*Scroll down for English version.*

---

## Magyar változat

Az **X-DOS** egy egyszerű, x86-alapú (32 bites védett módú) saját fejlesztésű operációs rendszer. Tartalmaz egy VGA szöveges felületet (80x25-ös felbontással), egy parancssort (shell), egy beépített szövegszerkesztőt (editor), RAM-alapú egyszerű fájlrendszert, valamint billentyűzet-kiszolgálót angol (US) és magyar (HU) kiosztások támogatásával.

### Tartalomjegyzék
1. [Rendszer Jellemzői](#rendszer-jellemzői)
2. [Projekt Struktúra](#projekt-struktúra)
3. [Előfeltételek](#előfeltételek)
4. [Telepítési Útmutató](#telepítési-útmutató)
5. [Fordítási Útmutató](#fordítási-útmutató)
6. [Futtatás](#futtatás)
7. [Parancssori Funkciók](#parancssori-funkciók)
8. [Szövegszerkesztő Gyorsbillentyűk](#szövegszerkesztő-gyorsbillentyűk)

### Rendszer Jellemzői
- **Architektúra**: 32 bites x86 Protected Mode (szegmentálás, IDT, PIC inicializálással).
- **Megjelenítés**: VGA szöveges mód (színes 80x25 soros megjelenítés, hardveres kurzor).
- **Billentyűzet**: IRQ1 megszakítás-alapú kezelés, támogatja az angol (US QWERTY) és magyar (HU QWERTZ) billentyűzetkiosztásokat.
- **Fájlrendszer**: Memóriában lévő (RAM) fájlrendszer legfeljebb 16 fájl számára, fájlonként max 4 KB mérettel.
- **Szövegszerkesztő**: Egyszerű teljes képernyős editor billentyűzet-vezérléssel és fájlmentési lehetőséggel.

### Projekt Struktúra
```text
Os/
├── boot/
│   └── bootloader.asm      # 16 bites bootloader (betölti a kernelt protected módba)
├── kernel/
│   ├── include/            # Fejlécfájlok (*.h)
│   │   ├── app.h
│   │   ├── editor.h
│   │   ├── fs.h
│   │   ├── io.h
│   │   ├── keyboard.h
│   │   ├── shell.h
│   │   ├── string.h
│   │   ├── types.h
│   │   └── vga.h
│   ├── src/                # Megvalósítások (*.c)
│   │   ├── editor.c
│   │   ├── fs.c
│   │   ├── io.c
│   │   ├── kernel.c
│   │   ├── keyboard.c
│   │   ├── shell.c
│   │   ├── string.c
│   │   └── vga.c
│   ├── kernel_entry.asm    # Rendszermag belépési pontja (Assembly wrapper)
│   └── kernel.ld           # Linker script a kernel.bin címzéséhez
├── Makefile                # Fordítási parancsok gyűjteménye
└── README.md               # Ez a leírás
```

### Előfeltételek

Az X-DOS fordításához és futtatásához egy x86-os keresztfordító eszközláncra és egy emulátorra van szükség.

#### Linux (Ubuntu/Debian)
Telepítsd a szükséges csomagokat:
```bash
sudo apt update
sudo apt install build-essential nasm qemu-system-x86
```

#### Windows
Windows alatt javasolt a **MSYS2** használata, vagy a szükséges eszközök kézi telepítése és a rendszerváltozókhoz (PATH) való hozzáadása:
1. **GCC** (32-bit target támogatással, pl. MinGW-w64 x86_64/i686)
2. **NASM** (Assembly fordító)
3. **Make** (Automata fordítóeszköz)
4. **QEMU** (Emulátor)
5. **dd** (Képfájl másoló segédprogram - MSYS2 vagy Git Bash részeként elérhető)

### Telepítési Útmutató

Klónozd vagy töltsd le a projektet a számítógépedre:
```bash
git clone <repository_url>
cd Os
```

Győződj meg róla, hogy a build eszközök elérhetők a terminálodból:
```bash
gcc --version
nasm -v
make --version
qemu-system-i386 --version
```

### Fordítási Útmutató

Az operációs rendszer fordítását a gyökérkönyvtárban lévő `Makefile` vezérli.

#### Teljes fordítás (Floppy lemezkép létrehozása)
Az alábbi parancs lefordítja a bootloadert, a kernelt és az összes C modult, majd összefűzi őket egy `myos.img` nevű 1.44 MB-os virtuális floppy lemezképpé:
```bash
make
```

#### Takarítás (Clean)
Az átmeneti objektumfájlok (`.o`) és a lemezképek eltávolításához:
```bash
make clean
```

### Futtatás

A lefordított lemezképet könnyedén elindíthatod QEMU emulátorban a Makefile beépített céljaival.

#### Futtatás Floppy lemezképként
```bash
make run
```
Ez a parancs elindítja a `qemu-system-i386` emulátort a lefordított `myos.img` lemezképpel.

#### Debug mód
Ha hibakeresést szeretnél végezni GDB használatával:
```bash
make debug
```

### Parancssori Funkciók

Miután az X-DOS betöltődött, az alábbi parancsokat használhatod a parancssorban (X-DOS>):

| Parancs | Leírás | Használat |
| :--- | :--- | :--- |
| `help` | Megjeleníti az elérhető parancsok listáját. | `help` |
| `clear` | Letörli a képernyőt és visszaállítja a kurzort a bal felső sarokba. | `clear` |
| `about` | Megjeleníti a rendszerinformációkat (architektúra, videó, billentyűzet). | `about` |
| `echo` | Kiírja a megadott szöveget a képernyőre. | `echo Szia Vilag!` |
| `ls` | Listázza a RAM-ban létrehozott fájlokat és azok méretét. | `ls` |
| `edit` | Megnyitja a megadott fájlt szerkesztésre a szövegszerkesztőben. | `edit dokumentum.txt` |
| `cat` | Kiírja a megadott fájl tartalmát a képernyőre. | `cat dokumentum.txt` |
| `rm` | Törli a megadott fájlt a memóriából. | `rm dokumentum.txt` |
| `set keyboard` | Átállítja a billentyűzet kiosztást angolra (`en`) vagy magyarra (`hu`). | `set keyboard hu` vagy `set keyboard en` |
| `reboot` | Szoftveresen újraindítja a virtuális gépet (hármas hibát előidézve). | `reboot` |

### Szövegszerkesztő Gyorsbillentyűk

Amikor az `edit <fajlnev>` paranccsal belépsz a szövegszerkesztő módba, a következő gyorsbillentyűket használhatod:

- **Navigáció**:
  - `Nyilak (Fel, Le, Balra, Jobbra)`: Kurzor mozgatása a szövegben.
  - `Home`: Kurzor ugrása a sor elejére.
  - `End`: Kurzor ugrása a sor végére.
- **Szerkesztés**:
  - `Backspace`: Karakter törlése a kurzor előtt.
  - `Delete`: Karakter törlése a kurzor után.
  - `Enter`: Új sor beszúrása.
- **Rendszer parancsok**:
  - `Ctrl + S`: Mentés a fájlrendszerbe (a RAM-ba).
  - `Ctrl + Q`: Kilépés vissza a parancssorba (a nem mentett változtatások elvesznek).
  - `Ctrl + N`: Új üres dokumentum létrehozása (`untitled.txt` néven).

---

## English version

**X-DOS** is a simple, custom-built x86-based (32-bit protected mode) operating system. It features a VGA text mode interface (80x25 resolution), a command shell, a built-in text editor, a simple RAM-based filesystem, and a keyboard driver supporting English (US) and Hungarian (HU) layouts.

### Table of Contents
1. [System Features](#system-features)
2. [Project Structure](#project-structure)
3. [Prerequisites](#prerequisites-1)
4. [Installation Guide](#installation-guide-1)
5. [Build Guide](#build-guide-1)
6. [Running the OS](#running-the-os)
7. [Shell Commands](#shell-commands)
8. [Text Editor Shortcuts](#text-editor-shortcuts)

### System Features
- **Architecture**: 32-bit x86 Protected Mode (with segmentation, IDT, and PIC initialization).
- **Display**: VGA text mode (color 80x25 character display with hardware cursor support).
- **Keyboard**: Interrupt-driven handling via IRQ1, supporting English (US QWERTY) and Hungarian (HU QWERTZ) keyboard layouts.
- **Filesystem**: A simple RAM-based filesystem supporting up to 16 files, with a maximum of 4 KB per file.
- **Text Editor**: A simple full-screen editor with keyboard navigation and file saving capabilities.

### Project Structure
```text
Os/
├── boot/
│   └── bootloader.asm      # 16-bit bootloader (loads kernel and switches to protected mode)
├── kernel/
│   ├── include/            # Header files (*.h)
│   │   ├── app.h
│   │   ├── editor.h
│   │   ├── fs.h
│   │   ├── io.h
│   │   ├── keyboard.h
│   │   ├── shell.h
│   │   ├── string.h
│   │   ├── types.h
│   │   └── vga.h
│   ├── src/                # Implementations (*.c)
│   │   ├── editor.c
│   │   ├── fs.c
│   │   ├── io.c
│   │   ├── kernel.c
│   │   ├── keyboard.c
│   │   ├── shell.c
│   │   ├── string.c
│   │   └── vga.c
│   ├── kernel_entry.asm    # Kernel entry point (Assembly wrapper)
│   └── kernel.ld           # Linker script for positioning kernel.bin
├── Makefile                # Collection of build commands
└── README.md               # This documentation
```

### Prerequisites

An x86 cross-compiler toolchain and an emulator are required to compile and run X-DOS.

#### Linux (Ubuntu/Debian)
Install the required packages:
```bash
sudo apt update
sudo apt install build-essential nasm qemu-system-x86
```

#### Windows
On Windows, it is recommended to use **MSYS2**, or manually install the required tools and add them to your environment variables (PATH):
1. **GCC** (with 32-bit target support, e.g., MinGW-w64 x86_64/i686)
2. **NASM** (Assembly compiler)
3. **Make** (Automated build tool)
4. **QEMU** (Emulator)
5. **dd** (Disk copier tool - available as part of MSYS2 or Git Bash)

### Installation Guide

Clone or download the project to your computer:
```bash
git clone <repository_url>
cd Os
```

Make sure the build tools are available in your terminal:
```bash
gcc --version
nasm -v
make --version
qemu-system-i386 --version
```

### Build Guide

Compiling the operating system is managed by the `Makefile` in the root directory.

#### Full compilation (Creating a Floppy disk image)
The following command compiles the bootloader, kernel, and all C modules, then merges them into a 1.44 MB virtual floppy disk image named `myos.img`:
```bash
make
```

#### Clean Build Outputs
To remove temporary object files (`.o`) and final image files:
```bash
make clean
```

### Running the OS

You can easily launch the compiled disk image in the QEMU emulator using the built-in targets in the Makefile.

#### Running as a Floppy Image
```bash
make run
```
This command starts the `qemu-system-i386` emulator with the compiled `myos.img` floppy image.

#### Debug Mode
If you want to debug the kernel using GDB:
```bash
make debug
```

### Shell Commands

Once X-DOS has loaded, you can use the following commands in the command prompt (X-DOS>):

| Command | Description | Usage |
| :--- | :--- | :--- |
| `help` | Shows the list of available commands. | `help` |
| `clear` | Clears the screen and resets the cursor to the top-left corner. | `clear` |
| `about` | Displays system details (architecture, video mode, keyboard layout). | `about` |
| `echo` | Prints the specified text to the screen. | `echo Hello World!` |
| `ls` | Lists the files created in RAM along with their sizes. | `ls` |
| `edit` | Opens the specified file for editing in the text editor. | `edit file.txt` |
| `cat` | Displays the content of the specified file on the screen. | `cat file.txt` |
| `rm` | Deletes the specified file from RAM. | `rm file.txt` |
| `set keyboard` | Switches the keyboard layout to English (`en`) or Hungarian (`hu`). | `set keyboard en` or `set keyboard hu` |
| `reboot` | Soft-reboots the virtual machine (by triggering a triple fault). | `reboot` |

### Text Editor Shortcuts

When entering the editor mode using the `edit <filename>` command, you can use the following shortcuts:

- **Navigation**:
  - `Arrows (Up, Down, Left, Right)`: Move the cursor through the text.
  - `Home`: Move the cursor to the start of the line.
  - `End`: Move the cursor to the end of the line.
- **Editing**:
  - `Backspace`: Delete the character before the cursor.
  - `Delete`: Delete the character after the cursor.
  - `Enter`: Insert a new line.
- **System Actions**:
  - `Ctrl + S`: Save the file to the RAM filesystem.
  - `Ctrl + Q`: Exit to the shell (unsaved changes will be lost).
  - `Ctrl + N`: Create a new empty document (named `untitled.txt`).
