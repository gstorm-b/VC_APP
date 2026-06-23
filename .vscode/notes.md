lưu ý khi build project:
- agent hãy dùng qmake để thay vì dùng trực tiếp mscv hay mingw
- Đây là build command mà mình thấy được khi chạy trên qtcreator:
    - Cho mingw
        Starting: "C:\Qt\Tools\llvm-mingw1706_64\bin\mingw32-make.exe" -j8
        C:/Qt/6.8.2/llvm-mingw_64/bin/qmake.exe -o Makefile ../../architecture_contract_test.pro -spec win32-clang-g++ CONFIG+=debug CONFIG+=qml_debug
        C:/Qt/Tools/llvm-mingw1706_64/bin/mingw32-make.exe -f Makefile.Debug
    - Cho msvc
        Starting: "C:\Qt\Tools\QtCreator\bin\jom\jom.exe" 
        C:\Qt\6.8.2\msvc2022_64\bin\qmake.exe -o Makefile ..\..\ncr_picking.pro -spec win32-msvc "CONFIG+=qtquickcompiler"
	    C:\Qt\Tools\QtCreator\bin\jom\jom.exe -f Makefile.Release        
- Có lẽ hai command trên đều run chạy khi directory đén folder build.