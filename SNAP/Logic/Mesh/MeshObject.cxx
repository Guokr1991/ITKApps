/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    MeshObject.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "MeshObject.h"

#include "ColorLabel.h"
#include "GlobalState.h"
#include "ImageWrapper.h"
#include "IRISApplication.h"
#include "IRISMeshPipeline.h"
#include "LevelSetMeshPipeline.h"
#include "IRISVectorTypesToITKConversion.h"
#include "IRISImageData.h"
#include "SNAPImageData.h"

// ITK includes
#include "itkRegionOfInterestImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkDiscreteGaussianImageFilter.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkRecursiveGaussianImageFilter.h"
#include "itkVTKImageExport.h"

// VTK includes
#include <vtkCellArray.h>
#include <vtkDecimatePro.h>
#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkContourFilter.h>
#include <vtkImageThreshold.h>
#include <vtkImageToStructuredPoints.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkStripper.h>

// System includes
#include <cstdlib>
#include "SNAPOpenGL.h"

using namespace itk;
using namespace std;

MeshObject
::MeshObject() 
{
  m_DisplayListNumber = 0;
  m_DisplayListIndex = 0;
}

void 
MeshObject
::Initialize(IRISApplication *driver)
{
  m_Driver = driver;
  m_GlobalState = m_Driver->GetGlobalState();
}

// Call this when segmentation data is no longer valid
void 
MeshObject
::Reset() 
{
  if (m_DisplayListNumber > 0) 
    {
    glDeleteLists(m_DisplayListIndex, m_DisplayListNumber);
    m_DisplayListNumber = 0;
    }
  m_Labels.clear();
}



void 
MeshObject
::GenerateMesh()
{
  // Reset the label array and the display list index
  Reset();

  // An array of meshes to be generated
  vector<vtkPolyData *> meshes;

  // The mesh is constructed differently depending on whether there is an
  // actively evolving level set or not
  // SNAP mode or in IRIS mode
  if (m_GlobalState->GetSnakeActive() && 
      m_Driver->GetSNAPImageData()->IsSnakeLoaded()) 
    {
    
    // We are in SNAP.  Use one of SNAP's images
    SNAPImageData *snapData = m_Driver->GetSNAPImageData();

    // Create a pipeline for mesh generation
    LevelSetMeshPipeline *meshPipeline = new LevelSetMeshPipeline();

    // Get the float distance transform from the level set mechanism
    meshPipeline->SetImage(snapData->GetLevelSetImage());
    
    // Compute the mesh only for the current segmentation color
    vtkPolyData *mesh = vtkPolyData::New();
    meshPipeline->ComputeMesh(mesh);
    meshes.push_back(mesh);
    
    // Deallocate the filter
    delete meshPipeline;
  }
  else    
    {
    // Create a pipeline for mesh generation
    IRISMeshPipeline *meshPipeline = new IRISMeshPipeline();
  
    // Initialize the pipeline with the correct image
    if(!m_GlobalState->GetSnakeActive())
      {
      // We are not currently in SNAP.  Use the segmentation image with its
      // different colors
      meshPipeline->SetImage(
        m_Driver->GetCurrentImageData()->GetSegmentation()->GetImage());    
  
      }
    else
      {
      // We are in SNAP.  Use one of SNAP's images
      SNAPImageData *snapData = m_Driver->GetSNAPImageData();
  
      /*
      // Select one of the available segmentation images in SNAP
      if(snapData->IsSnakeLoaded()) 
        {
        meshPipeline->SetImage(snapData->GetSnake()->GetImage());
        }
      else 
        {  */
      meshPipeline->SetImage(snapData->GetSegmentation()->GetImage());
        // }
      }
  
    // Pass the settings on to the pipeline
    meshPipeline->SetMeshOptions(m_GlobalState->GetMeshOptions());
  
    // Run the first step in this pipeline
    meshPipeline->ComputeBoundingBoxes();
  
    // Compute a list of meshes in the filter
    for(unsigned int i=1;i<MAX_COLOR_LABELS;i++) 
      {
      ColorLabel cl = m_Driver->GetCurrentImageData()->GetColorLabel(i);
      if(cl.IsDoMesh() && meshPipeline->CanComputeMesh(i))
        {
        vtkPolyData *mesh = vtkPolyData::New();
        meshPipeline->ComputeMesh(i,mesh);
        meshes.push_back(mesh);
  
        m_Labels.push_back(i);
        }
      }
    
    // Deallocate the filter
    delete meshPipeline;
    }


  // Generate an array of display lists
  m_DisplayListNumber = meshes.size();
  m_DisplayListIndex = glGenLists(m_DisplayListNumber);
  
  // Create a display list for each of the objects in the pipeline
  for(unsigned int dl=0;dl<m_DisplayListNumber;dl++)
    {
    // Get the triangle strip information.
    vtkCellArray *triStrips = meshes[dl]->GetStrips();

    // Get the vertex information.
    vtkPoints *verts = meshes[dl]->GetPoints();

    // Get the normal information.
    vtkDataArray *norms = meshes[dl]->GetPointData()->GetNormals();
    
    // Build display list
    glNewList(m_DisplayListIndex + dl,GL_COMPILE);

    vtkIdType ntris = 0;
    vtkIdType npts;
    vtkIdType *pts;
    for ( triStrips->InitTraversal(); triStrips->GetNextCell(npts,pts); ) 
      {
      ntris += npts-2;
      glBegin( GL_TRIANGLE_STRIP );
      for (vtkIdType j = 0; j < npts; j++) 
        {
        // Some ugly code to ensure VTK version compatibility
        double vx = verts->GetPoint(pts[j])[0];
        double vy = verts->GetPoint(pts[j])[1];
        double vz = verts->GetPoint(pts[j])[2];
        double nx = norms->GetTuple(pts[j])[0];
        double ny = norms->GetTuple(pts[j])[1];
        double nz = norms->GetTuple(pts[j])[2];
        
        // Specify normal.
        glNormal3d(nx, ny, nz);

        // Specify vertex.
        glVertex3d(vx, vy, vz);
      }
      glEnd();
    }
    glEndList();

    // Delete the mesh
    meshes[dl]->Delete();    
    }
}

/*
 *  Apply color label, a shorthand
 */
bool 
MeshObject
::ApplyColorLabel(const ColorLabel &label) 
{
  if (label.IsVisible() && label.IsDoMesh()) 
    {
    // Adjust the label color to reduce the saturation. This in necessary
    // in order to see the highlights on the object
    double r = 0.75 * (label.GetRGB(0) / 255.0);
    double g = 0.75 * (label.GetRGB(1) / 255.0);
    double b = 0.75 * (label.GetRGB(2) / 255.0);
    double a = label.GetAlpha() / 255.0;

    if (label.IsOpaque()) glColor3d(r, g, b); 
    else glColor4d(r, g, b, a);
  
    return true;
    }
  return false;
}

void 
MeshObject
::Display() 
{
  unsigned int i;

  // Define a shorthand pointer to the image data
  IRISImageData *irisData = m_Driver->GetCurrentImageData();  
  
  // check if the snake is not active
  // if yes it means we are in IRIS 
  // so we render all segmentations
  if (!m_GlobalState->GetSnakeActive()) 
    {
    
    // First render all the fully opaque objects
    for (i=0; i < m_DisplayListNumber; i++) 
      {
      const ColorLabel &cl = irisData->GetColorLabel(m_Labels[i]);
      if (cl.IsOpaque() && ApplyColorLabel(cl)) 
      {
        glCallList(m_DisplayListIndex + i);
      }
    }

    // Now render all the translucent objects, with depth buffer read-only
    // Ideally we should sort polygons from back to front
    // TODO: Utilize VTK!
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    
    glEnable(GL_BLEND);     
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);     
    glDepthMask(GL_FALSE); 
    
    for (i=0; i < m_DisplayListNumber; i++) 
      {
      const ColorLabel &cl = irisData->GetColorLabel(m_Labels[i]);
      if (!cl.IsOpaque() && ApplyColorLabel(cl)) 
      {
        glCallList(m_DisplayListIndex + i); 
      }
    }

    glPopAttrib();
  }

  // if the snake is active render only the current segmentation
  // added by Konstantin Bobkov
  else if(m_DisplayListNumber > 0) 
  {
    // get current label
    int currentcolor =  m_GlobalState->GetDrawingColorLabel();

    // first check if the segmentation is fully opaque
    // and render it
    const ColorLabel &cl = irisData->GetColorLabel(currentcolor);
    if (cl.IsOpaque() && ApplyColorLabel(cl)) 
    {
      glCallList(m_DisplayListIndex);
    }

    // now check if the segmentation is translucent 
    // and render it with depth buffer read-only
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    if (!cl.IsOpaque() && ApplyColorLabel(cl)) 
    {
      glCallList(m_DisplayListIndex);  
    }

    glPopAttrib();
    
  }
}

/*
 *Log: MeshObject.cxx
 *Revision 1.11  2004/01/27 17:34:00  pauly
 *FIX: Compiling on Mac OSX, issue with GLU include file
 *
 *Revision 1.10  2004/01/20 00:17:42  pauly
 *FIX: VTK float compatibility
 *
 *Revision 1.9  2004/01/17 18:39:07  lorensen
 *ENH: changes to accomodate VTK api changes.
 *
 *Revision 1.8  2003/10/09 22:45:13  pauly
 *EMH: Improvements in 3D functionality and snake parameter preview
 *
 *Revision 1.7  2003/10/02 14:54:53  pauly
 *ENH: Development during the September code freeze
 *
 *Revision 1.1  2003/09/11 13:50:29  pauly
 *FIX: Enabled loading of images with different orientations
 *ENH: Implemented image save and load operations
 *
 *Revision 1.5  2003/08/28 14:37:09  pauly
 *FIX: Clean 'unused parameter' and 'static keyword' warnings in gcc.
 *FIX: Label editor repaired
 *
 *Revision 1.4  2003/08/27 14:03:21  pauly
 *FIX: Made sure that -Wall option in gcc generates 0 warnings.
 *FIX: Removed 'comment within comment' problem in the cvs log.
 *
 *Revision 1.3  2003/08/27 04:57:46  pauly
 *FIX: A large number of bugs has been fixed for 1.4 release
 *
 *Revision 1.2  2003/08/14 13:37:07  pauly
 *FIX: Error with using int instead of vtkIdType in calling vtkCellArray::GetNextCell
 *
 *Revision 1.1  2003/07/12 04:52:25  pauly
 *Initial checkin of SNAP application  to the InsightApplications tree
 *
 *Revision 1.2  2003/07/12 01:34:18  pauly
 *More final changes before ITK checkin
 *
 *Revision 1.1  2003/07/11 23:29:17  pauly
 **** empty log message ***
 *
 *Revision 1.10  2003/07/11 21:25:12  pauly
 *Code cleanup for ITK checkin
 *
 *Revision 1.9  2003/07/10 14:30:26  pauly
 *Integrated ITK into SNAP level set segmentation
 *
 *Revision 1.8  2003/06/08 16:11:42  pauly
 *User interface changes
 *Automatic mesh updating in SNAP mode
 *
 *Revision 1.7  2003/05/12 02:51:10  pauly
 *Got code to compile on UNIX
 *
 *Revision 1.6  2003/05/05 12:30:18  pauly
 **** empty log message ***
 *
 *Revision 1.5  2003/04/29 14:01:42  pauly
 *Charlotte Trip
 *
 *Revision 1.4  2003/04/23 20:36:23  pauly
 **** empty log message ***
 *
 *Revision 1.3  2003/04/23 06:05:18  pauly
 **** empty log message ***
 *
 *Revision 1.2  2003/04/18 17:32:18  pauly
 **** empty log message ***
 *
 *Revision 1.1  2003/03/07 19:29:47  pauly
 *Initial checkin
 *
 *Revision 1.2  2002/12/16 16:40:19  pauly
 **** empty log message ***
 *
 *Revision 1.1.1.1  2002/12/10 01:35:36  pauly
 *Started the project repository
 *
 *
 *Revision 1.11  2002/05/08 17:34:18  moon
 *Fixed bug of using the voxel snake image, instead of hte float snake state,
 *for the marching cubes. Now the snake looks smooth instead of voxelized.
 *
 *Revision 1.10  2002/04/27 18:30:22  moon
 *Finished commenting
 *
 *Revision 1.9  2002/04/27 17:49:00  bobkov
 *Added comments
 *
 *Revision 1.8  2002/04/24 17:12:48  bobkov
 *modified display() method so that when num_lists is zero
 *the 3D window does not show anything
 *
 *Revision 1.7  2002/04/23 19:30:10  bobkov
 *modified display method so that the current color label is
 *used in the snake 3d window
 *
 *Revision 1.6  2002/04/13 17:45:32  moon
 *I put code in to check if a label exists in the seg image in GenerateMesh,
 *so that the vtk pipeline (SLOW!) wouldn't have to get executed on empty images.
 *(i.e. all 6 or whatever labels that are initialized at the start are "rendered"
 *even if only one label has actually been used)
 *
 *Revision 1.5  2002/04/10 21:20:42  moon
 *fixed bug when update mesh is pressed with no seg data
 *
 *Revision 1.4  2002/04/10 20:20:36  moon
 *fixed small problem with snake 3d window mesh generation with Konstantin
 *It was using full_data, and we switched it to use only roi_data.
 *
 *Revision 1.3  2002/04/09 22:00:23  bobkov
 *
 *Modified GenerateMesh method to display 3d snake segmentation
 *
 *Revision 1.2  2002/03/08 14:06:29  moon
 *Added Header and Log tags to all files
 **/
