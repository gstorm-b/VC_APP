
Unit tests output 1:
    {
        ********* Start testing of TestCalibrationBoard *********
        Config: Using QtTest library 6.8.3, Qt 6.8.3 (x86_64-little_endian-llp64 shared (dynamic) debug build; by MSVC 2022), windows 11
        PASS   : TestCalibrationBoard::initTestCase()
        PASS   : TestCalibrationBoard::generateImage_hasExpectedSize()
        PASS   : TestCalibrationBoard::objectPoints_countAndSpacing()
        PASS   : TestCalibrationBoard::detect_onGeneratedImage_returnsAllDots()
        PASS   : TestCalibrationBoard::detect_orientationOrder_isRowMajor()
        PASS   : TestCalibrationBoard::cleanupTestCase()
        Totals: 6 passed, 0 failed, 0 skipped, 0 blacklisted, 363ms
        ********* Finished testing of TestCalibrationBoard *********
        ********* Start testing of TestCalibrator *********
        Config: Using QtTest library 6.8.3, Qt 6.8.3 (x86_64-little_endian-llp64 shared (dynamic) debug build; by MSVC 2022), windows 11
        PASS   : TestCalibrator::initTestCase()
        PASS   : TestCalibrator::calibrate_lessThan4Points_returnsFalse()
        A crash occurred in C:\DGB\Project\calib_board_recognize\tests\build\Desktop_Qt_6_8_3_MSVC2022_64bit-Debug\debug\calib_tests.exe.
        While testing calibrate_perfectAffine_lowError
        Function time: 2267ms Total time: 2269ms

        Exception address: 0x00007FF7689890CD
        Exception code   : 0x80000003
        Nearby symbol    : std::normal_distribution<float>::param_type::_Init

        Stack:
        #  1: QTestLog::enterTestFunction() - 0x00007FF9780C5CC2
        #  2: UnhandledExceptionFilter() - 0x00007FF99792BAD0
        #  3: strncpy() - 0x00007FF99A405B90
        #  4: _C_specific_handler() - 0x00007FF99A3C3D40
        #  5: _chkstk() - 0x00007FF99A403E70
        #  6: RtlWow64GetCurrentCpuArea() - 0x00007FF99A2B30E0
        #  7: KiUserExceptionDispatcher() - 0x00007FF99A403820
        #  8: std::normal_distribution<float>::param_type::_Init() - 0x00007FF768989070
        #  9: std::normal_distribution<float>::param_type::param_type() - 0x00007FF768987E30
        # 10: std::normal_distribution<float>::normal_distribution<float>() - 0x00007FF768987880
        # 11: `anonymous namespace'::applyAffine() - 0x00007FF768984C80
        # 12: TestCalibrator::calibrate_perfectAffine_lowError() - 0x00007FF768983DA0
        # 13: TestCalibrator::qt_static_metacall() - 0x00007FF7689839E0
        # 14: QFileInfo::filePath() - 0x00007FF8D51F66CE
        # 15: QFileInfo::filePath() - 0x00007FF8D51F66CE
        # 16: QTestLog::enterTestFunction() - 0x00007FF9780C5CC2
        # 17: QTestLog::enterTestFunction() - 0x00007FF9780C5CC2
        # 18: QTestLog::enterTestFunction() - 0x00007FF9780C5CC2
        # 19: QTestLog::enterTestFunction() - 0x00007FF9780C5CC2
        # 20: QTestLog::enterTestFunction() - 0x00007FF9780C5CC2
        # 21: QTestLog::enterTestFunction() - 0x00007FF9780C5CC2
        # 22: QTestLog::enterTestFunction() - 0x00007FF9780C5CC2
        # 23: QTestLog::enterTestFunction() - 0x00007FF9780C5CC2
        # 24: QTestLog::enterTestFunction() - 0x00007FF9780C5CC2
        # 25: QTestLog::enterTestFunction() - 0x00007FF9780C5CC2
        # 26: main() - 0x00007FF768984ED0
        # 27: invoke_main() - 0x00007FF768998360
        # 28: __scrt_common_main_seh() - 0x00007FF768998110
        # 29: __scrt_common_main() - 0x00007FF7689980F0
        # 30: mainCRTStartup() - 0x00007FF768998420
        # 31: BaseThreadInitThunk() - 0x00007FF99810E8C0
        # 32: RtlUserThreadStart() - 0x00007FF99A35BF00
    }

Unit test output 2:
    {
        ********* Start testing of TestCalibrationBoard *********
        Config: Using QtTest library 6.8.3, Qt 6.8.3 (x86_64-little_endian-llp64 shared (dynamic) debug build; by MSVC 2022), windows 11
        PASS   : TestCalibrationBoard::initTestCase()
        PASS   : TestCalibrationBoard::generateImage_hasExpectedSize()
        PASS   : TestCalibrationBoard::objectPoints_countAndSpacing()
        PASS   : TestCalibrationBoard::detect_onGeneratedImage_returnsAllDots()
        PASS   : TestCalibrationBoard::detect_orientationOrder_isRowMajor()
        PASS   : TestCalibrationBoard::cleanupTestCase()
        Totals: 6 passed, 0 failed, 0 skipped, 0 blacklisted, 363ms
        ********* Finished testing of TestCalibrationBoard *********
        ********* Start testing of TestCalibrator *********
        Config: Using QtTest library 6.8.3, Qt 6.8.3 (x86_64-little_endian-llp64 shared (dynamic) debug build; by MSVC 2022), windows 11
        PASS   : TestCalibrator::initTestCase()
        PASS   : TestCalibrator::calibrate_lessThan4Points_returnsFalse()
        PASS   : TestCalibrator::calibrate_perfectAffine_lowError()
        PASS   : TestCalibrator::roundTrip_imageToRobotToImage()
        PASS   : TestCalibrator::saveLoad_homographyPreserved()
        PASS   : TestCalibrator::cleanupTestCase()
        Totals: 6 passed, 0 failed, 0 skipped, 0 blacklisted, 34ms
        ********* Finished testing of TestCalibrator *********
        [ INFO:0@0.025] global registry_parallel.impl.hpp:96 cv::parallel::ParallelBackendRegistry::ParallelBackendRegistry core(parallel): Enabled backends(3, sorted by priority): ONETBB(1000); TBB(990); OPENMP(980)
        [ INFO:0@0.025] global plugin_loader.impl.hpp:67 cv::plugin::impl::DynamicLib::libraryLoad load C:\opencv\build\x64\vc16\bin\opencv_core_parallel_onetbb4110_64d.dll => FAILED
        [ INFO:0@0.026] global plugin_loader.impl.hpp:67 cv::plugin::impl::DynamicLib::libraryLoad load opencv_core_parallel_onetbb4110_64d.dll => FAILED
        [ INFO:0@0.026] global plugin_loader.impl.hpp:67 cv::plugin::impl::DynamicLib::libraryLoad load C:\opencv\build\x64\vc16\bin\opencv_core_parallel_tbb4110_64d.dll => FAILED
        [ INFO:0@0.027] global plugin_loader.impl.hpp:67 cv::plugin::impl::DynamicLib::libraryLoad load opencv_core_parallel_tbb4110_64d.dll => FAILED
        [ INFO:0@0.027] global plugin_loader.impl.hpp:67 cv::plugin::impl::DynamicLib::libraryLoad load C:\opencv\build\x64\vc16\bin\opencv_core_parallel_openmp4110_64d.dll => FAILED
        [ INFO:0@0.028] global plugin_loader.impl.hpp:67 cv::plugin::impl::DynamicLib::libraryLoad load opencv_core_parallel_openmp4110_64d.dll => FAILED
    }

    
Output main 1:
    {
        === CalibrationBoard ===
        rows=9  cols=11  dot=4mm  spacing=15mm
        Saved board.png  (1900x1600 px)
        [ INFO:0@0.049] global registry_parallel.impl.hpp:96 cv::parallel::ParallelBackendRegistry::ParallelBackendRegistry core(parallel): Enabled backends(3, sorted by priority): ONETBB(1000); TBB(990); OPENMP(980)
        [ INFO:0@0.049] global plugin_loader.impl.hpp:67 cv::plugin::impl::DynamicLib::libraryLoad load C:\opencv\build\x64\vc16\bin\opencv_core_parallel_onetbb4110_64d.dll => FAILED
        [ INFO:0@0.050] global plugin_loader.impl.hpp:67 cv::plugin::impl::DynamicLib::libraryLoad load opencv_core_parallel_onetbb4110_64d.dll => FAILED
        [ INFO:0@0.050] global plugin_loader.impl.hpp:67 cv::plugin::impl::DynamicLib::libraryLoad load C:\opencv\build\x64\vc16\bin\opencv_core_parallel_tbb4110_64d.dll => FAILED
        [ INFO:0@0.051] global plugin_loader.impl.hpp:67 cv::plugin::impl::DynamicLib::libraryLoad load opencv_core_parallel_tbb4110_64d.dll => FAILED
        [ INFO:0@0.051] global plugin_loader.impl.hpp:67 cv::plugin::impl::DynamicLib::libraryLoad load C:\opencv\build\x64\vc16\bin\opencv_core_parallel_openmp4110_64d.dll => FAILED
        [ INFO:0@0.051] global plugin_loader.impl.hpp:67 cv::plugin::impl::DynamicLib::libraryLoad load opencv_core_parallel_openmp4110_64d.dll => FAILED
        Detection: OK  detected=99 / expected=99
        Saved board_detected.png

        === Calibrator ===
        samples = 99
        reprojection error: 0.706356 px,  0.0706276 mm
        H (image px -> robot mm) (3x3):
        [     0.099100    -0.013143   232.826095 ]
        [     0.013043     0.099140  -142.425931 ]
        [     0.000000    -0.000000     1.000000 ]

        --- Sample conversions ---
        image center  : (950.000000, 800.000000) px
        -> robot    : (316.524902, -50.733871) mm
        -> back img : (950.000122, 800.000000) px

        Saved calibration.yml
        Reloaded H, image-center -> robot = (316.524902, -50.733871) mm
    }