#include <QtTest/QtTest>

#include <iostream>
#include <string>
#include <vector>

int runSmokeTests(int argc, char** argv);
int runDhAdapterTests(int argc, char** argv);
int runUnitsTests(int argc, char** argv);
int runPoseTests(int argc, char** argv);
int runRobotModelConfigTests(int argc, char** argv);
int runRobotModelValidatorTests(int argc, char** argv);
int runJointLimitValidatorTests(int argc, char** argv);
int runFrameToolTests(int argc, char** argv);
int runForwardKinematicsTests(int argc, char** argv);
int runRobot3DVisualizerLogicTests(int argc, char** argv);
int runFrameToolFkTests(int argc, char** argv);
int runIKApiTests(int argc, char** argv);
int runCollisionApiTests(int argc, char** argv);
int runCollisionBackendTests(int argc, char** argv);
int runCollisionProfileValidatorTests(int argc, char** argv);
int runCollisionCheckerTests(int argc, char** argv);
int runCollisionPrimitiveDistanceTests(int argc, char** argv);
int runStlMeshLoaderTests(int argc, char** argv);
int runStlPrimitiveAuthoringHelperTests(int argc, char** argv);
int runIKSolutionRankerTests(int argc, char** argv);
int runNumericalIKSolverTests(int argc, char** argv);
int runPostureResolverTests(int argc, char** argv);
int runCustomPresetTests(int argc, char** argv);
int runCollisionProfileJsonTests(int argc, char** argv);
int runMeshCollisionProfileJsonTests(int argc, char** argv);
int runNachiMeshCollisionTests(int argc, char** argv);
int runIKIntegrationTests(int argc, char** argv);
int runVirtual6DofTestArmTests(int argc, char** argv);
int runNachiMZ04DTests(int argc, char** argv);
int runAnalyticIKSolverTests(int argc, char** argv);
int runUrdfAdapterTests(int argc, char** argv);

namespace {
struct NamedSuite
{
    const char* name;
    int (*suite)(int, char**);
};

int runSuite(const char* name, int (*suite)(int, char**), int argc, char** argv)
{
    std::cout << "Running " << name << "..." << std::endl;
    const int failures = suite(argc, argv);
    std::cout << name << ": " << (failures == 0 ? "PASS" : "FAIL");
    if (failures != 0) {
        std::cout << " (" << failures << " failing test function(s))";
    }
    std::cout << std::endl;
    return failures;
}

bool suiteMatchesFilter(const char* suiteName, const std::string& filter)
{
    return !filter.empty() && filter == suiteName;
}
}

int main(int argc, char** argv)
{
    const std::vector<NamedSuite> suites = {
        {"SmokeTests", runSmokeTests},
        {"DhAdapterTests", runDhAdapterTests},
        {"UnitsTests", runUnitsTests},
        {"PoseTests", runPoseTests},
        {"RobotModelConfigTests", runRobotModelConfigTests},
        {"RobotModelValidatorTests", runRobotModelValidatorTests},
        {"JointLimitValidatorTests", runJointLimitValidatorTests},
        {"FrameToolTests", runFrameToolTests},
        {"ForwardKinematicsTests", runForwardKinematicsTests},
        {"Robot3DVisualizerLogicTests", runRobot3DVisualizerLogicTests},
        {"FrameToolFkTests", runFrameToolFkTests},
        {"IKApiTests", runIKApiTests},
        {"CollisionApiTests", runCollisionApiTests},
        {"CollisionBackendTests", runCollisionBackendTests},
        {"CollisionProfileValidatorTests", runCollisionProfileValidatorTests},
        {"CollisionCheckerTests", runCollisionCheckerTests},
        {"CollisionPrimitiveDistanceTests", runCollisionPrimitiveDistanceTests},
        {"StlMeshLoaderTests", runStlMeshLoaderTests},
        {"StlPrimitiveAuthoringHelperTests", runStlPrimitiveAuthoringHelperTests},
        {"IKSolutionRankerTests", runIKSolutionRankerTests},
        {"NumericalIKSolverTests", runNumericalIKSolverTests},
        {"PostureResolverTests", runPostureResolverTests},
        {"CustomPresetTests", runCustomPresetTests},
        {"CollisionProfileJsonTests", runCollisionProfileJsonTests},
        {"MeshCollisionProfileJsonTests", runMeshCollisionProfileJsonTests},
        {"NachiMeshCollisionTests", runNachiMeshCollisionTests},
        {"IKIntegrationTests", runIKIntegrationTests},
        {"Virtual6DofTestArmTests", runVirtual6DofTestArmTests},
        {"NachiMZ04DTests", runNachiMZ04DTests},
        {"AnalyticIKSolverTests", runAnalyticIKSolverTests},
        {"UrdfAdapterTests", runUrdfAdapterTests},
    };

    std::string suiteFilter;
    std::vector<char*> forwardedArgs;
    forwardedArgs.reserve(static_cast<std::size_t>(argc));
    if (argc > 0) {
        forwardedArgs.push_back(argv[0]);
    }
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        bool matchedSuite = false;
        for (const NamedSuite& suite : suites) {
            if (argument == suite.name) {
                suiteFilter = argument;
                matchedSuite = true;
                break;
            }
        }
        if (!matchedSuite) {
            forwardedArgs.push_back(argv[index]);
        }
    }

    int forwardedArgc = static_cast<int>(forwardedArgs.size());
    char** forwardedArgv = forwardedArgs.data();

    int status = 0;
    for (const NamedSuite& suite : suites) {
        if (!suiteFilter.empty() && !suiteMatchesFilter(suite.name, suiteFilter)) {
            continue;
        }
        status |= runSuite(suite.name, suite.suite, forwardedArgc, forwardedArgv);
    }
    return status;
}
