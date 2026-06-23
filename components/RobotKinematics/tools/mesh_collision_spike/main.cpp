#include <vtkCollisionDetectionFilter.h>
#include <vtkCubeSource.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkTriangleFilter.h>

#include <iostream>

namespace {

int contactCountForOffset(const double offsetX)
{
    vtkNew<vtkCubeSource> meshA;
    meshA->SetXLength(1.0);
    meshA->SetYLength(1.0);
    meshA->SetZLength(1.0);

    vtkNew<vtkCubeSource> meshB;
    meshB->SetXLength(1.0);
    meshB->SetYLength(1.0);
    meshB->SetZLength(1.0);

    vtkNew<vtkTriangleFilter> trianglesA;
    trianglesA->SetInputConnection(meshA->GetOutputPort());
    trianglesA->Update();

    vtkNew<vtkTriangleFilter> trianglesB;
    trianglesB->SetInputConnection(meshB->GetOutputPort());
    trianglesB->Update();

    vtkNew<vtkMatrix4x4> matrixA;
    matrixA->Identity();

    vtkNew<vtkMatrix4x4> matrixB;
    matrixB->Identity();
    matrixB->SetElement(0, 3, offsetX);

    vtkNew<vtkCollisionDetectionFilter> collision;
    collision->SetInputData(0, trianglesA->GetOutput());
    collision->SetInputData(1, trianglesB->GetOutput());
    collision->SetMatrix(0, matrixA);
    collision->SetMatrix(1, matrixB);
    collision->SetBoxTolerance(0.0f);
    collision->SetCellTolerance(0.0);
    collision->SetCollisionModeToAllContacts();
    collision->GenerateScalarsOff();
    collision->Update();

    return collision->GetNumberOfContacts();
}

}

int main()
{
    const int collidingContacts = contactCountForOffset(0.25);
    const int separatedContacts = contactCountForOffset(3.0);

    std::cout << "vtk_debug colliding_contacts=" << collidingContacts << std::endl;
    std::cout << "vtk_debug separated_contacts=" << separatedContacts << std::endl;

    if (collidingContacts <= 0) {
        std::cerr << "[ERROR] Expected a colliding query to report contacts." << std::endl;
        return 1;
    }
    if (separatedContacts != 0) {
        std::cerr << "[ERROR] Expected a separated query to report zero contacts." << std::endl;
        return 1;
    }

    std::cout << "[OK] VTK debug spike query passed." << std::endl;
    return 0;
}
