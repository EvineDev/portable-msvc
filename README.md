# Generate portable MSVC

### Generate
* Start `x64 Native Tools Command Prompt`, alternatively run `vcvarsall.bat` in a command prompt
* Run `cl generate.c`
* Run `generate.exe`

Now you have a folder called `msvc` that contains all required files for msvc `cl`, `link`, `ml64`, `dumpbin` and etc... to run from the command line.

To initialize the portable install you run `vcvarsportable.bat` inside `msvc` directory or you can install it as descibed below.

### Install to enviorment variables

**Warning:** Generating new portable msvc's might grab files from this portable installation. Make sure you undo these steps before generating new portable msvc's. This is important to know when you update visual studio and want to update the portable install as well.

if you want to run `cl` etc, without `x64 Native Tools Command Prompt`, `vcvarsall.bat` or `vcvarsportable.bat` you can install the portable installation to the system enviorment variables.

* I'd recommend to move the installation directory to an easily accessible location. Eg. `C:\msvc`
* Append the full path `msvc` directory to `PATH` Eg. `C:\msvc`
* Create new enviorment variable `INCLUDE` and add the full path to it. Eg. `C:\msvc\include`
* Create new enviorment variable `LIB` and add the full path to it. Eg. `C:\msvc\lib`
* Create new enviorment variable `LIBPATH` and add the full path to it. Eg. `C:\msvc\libpath`
