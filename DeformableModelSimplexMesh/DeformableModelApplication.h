#ifndef __DeformableModelApplication__h__
#define __DeformableModelApplication__h__


#include "DeformableModelApplicationGUI.h"
#include "ImageSliceViewer.h"
#include "ImageSurfaceViewer.h"
#include "itkVertexCell.h"
#include "itkLineCell.h"

#include <itkTriangleCell.h>
#include <itkCellInterface.h>
#include <vtkCellArray.h>

class vtkImageShiftScale;

class VistVTKCellsClass
{
  vtkCellArray* m_Cells;
  int* m_LastCell;
  int* m_TypeArray;
  

   public:

  typedef DeformableModelApplicationBase::SimplexMeshType    SimplexMeshType;

  //typedef the itk cells we are interested in
  typedef itk::CellInterface< SimplexMeshType::PixelType, 
                              SimplexMeshType::CellTraits >  CellInterfaceType;
  typedef itk::VertexCell<CellInterfaceType>                 VertexCell;
  typedef itk::LineCell<CellInterfaceType>                   LineCell;

  /*! 
    Set the vtkCellArray that will be constructed
   */
  void SetCellArray(vtkCellArray* a) 
    {
      m_Cells = a;
    }
  
  /*! 
    Set the cell counter pointer
  */
  void SetCellCounter(int* i)
    {
      m_LastCell = i;
    }

  /*!
    Set the type array for storing the vtk cell types
  */
  void SetTypeArray(int* i)
    {
      m_TypeArray = i;
    }

  /*!
    Visit a line and create the VTK_LINE cell   
  */
  void Visit(unsigned long , LineCell* t)
    {
      m_Cells->InsertNextCell(2,  (vtkIdType*)t->PointIdsBegin());
      m_TypeArray[*m_LastCell] = VTK_LINE;
      (*m_LastCell)++;
    }
  
};
  
class DeformableModelApplication : public DeformableModelApplicationGUI
{
 public:
  
  DeformableModelApplication();
  virtual ~DeformableModelApplication();
  
  typedef itk::PointSet< double, 3 > PointSetType;
  
  virtual void Show();
  virtual void Hide();
  virtual void Quit();
  virtual void SaveMesh();
  virtual void LoadMesh();
  virtual void CreateMesh();
  virtual void RefreshMeshVisualization();
  virtual void DeformMesh();
  virtual void ComputeInternalForces();
  virtual void Load();
  virtual void LoadPostProcessing();
  virtual void SelectAxialSlice( int );
  virtual void SelectCoronalSlice( int );
  virtual void SelectSagittalSlice( int );
  virtual void ProcessAxialViewInteraction( void );
  virtual void ProcessCoronalViewInteraction( void );
  virtual void ProcessSagittalViewInteraction( void );
  virtual void ProcessSurfaceViewInteraction( void );
  
  virtual void SyncAllViews(void);
  virtual void IterationCallback();


  typedef itk::CellInterfaceVisitorImplementation< SimplexMeshType::PixelType,
                                                   SimplexMeshType::CellTraits,
                                                   lineType, 
                                                   VistVTKCellsClass>  LineVisitor;

  
   
 private:

  ImageSliceViewer  m_AxialViewer;
  ImageSliceViewer  m_CoronalViewer;
  ImageSliceViewer  m_SagittalViewer;
  ImageSurfaceViewer  m_SimplexMeshViewer;
  ImageSliceViewer  m_Dummy;

  vtkImageShiftScale    * m_ShiftScaleImageFilter;

  itk::SimpleMemberCommand<DeformableModelApplication>::Pointer      m_AxialViewerCommand;
  itk::SimpleMemberCommand<DeformableModelApplication>::Pointer      m_CoronalViewerCommand;
  itk::SimpleMemberCommand<DeformableModelApplication>::Pointer      m_SagittalViewerCommand;
  itk::SimpleMemberCommand<DeformableModelApplication>::Pointer      m_SurfaceViewerCommand;
  
  bool m_InternalForcesComputed;

};


#endif
