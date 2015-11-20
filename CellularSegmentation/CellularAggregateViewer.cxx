/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    CellularAggregateViewer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4503 )
#endif
#include <fstream>
#include "CellularAggregateViewer.h"

#include "vtkLine.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkDelaunay2D.h"
#include "vtkGlyph2D.h"
#include "vtkProperty.h"
#include "vtkCylinderSource.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"


namespace itk {

namespace bio {




CellularAggregateViewer
::CellularAggregateViewer()
{
  m_Renderer = NULL;

  m_SurfaceActor  = vtkActor::New();
  m_SurfaceMapper = vtkPolyDataMapper::New();
  m_PolyData      = vtkPolyData::New();

  m_Delaunay2DFilter = vtkDelaunay2D::New();

  m_CellGlyphSource = vtkCylinderSource::New();
  m_CellGlyphSource->SetResolution(12);

  m_CellGlyphTransform = vtkTransform::New();
  m_CellGlyphTransform->RotateX( 90.0 );

  m_CellGlyphTransformFilter = vtkTransformPolyDataFilter::New();
  m_CellGlyphTransformFilter->SetTransform( m_CellGlyphTransform );
#if VTK_MAJOR_VERSION <= 5
  m_CellGlyphTransformFilter->SetInput( m_CellGlyphSource->GetOutput() );
#else
  m_CellGlyphSource->Update();
  m_CellGlyphTransformFilter->SetInputData( m_CellGlyphSource->GetOutput() );
#endif

  m_GlyphFilter = vtkGlyph2D::New();
#if VTK_MAJOR_VERSION <= 5
  m_GlyphFilter->SetSource( m_CellGlyphTransformFilter->GetOutput() );
#else
  m_CellGlyphTransformFilter->Update();
  m_GlyphFilter->SetSourceData( m_CellGlyphTransformFilter->GetOutput() );
#endif

#if VTK_MAJOR_VERSION <= 5
  m_SurfaceMapper->SetInput( m_PolyData );
#else
  m_SurfaceMapper->SetInputData( m_PolyData );
#endif

  m_SurfaceActor->SetMapper( m_SurfaceMapper );
  m_SurfaceActor->GetProperty()->SetColor(1.0,0.0,0.0);

  m_DisplayOption = CellWalls;
}




CellularAggregateViewer
::~CellularAggregateViewer()
{
}


void
CellularAggregateViewer
::Draw(void) const
{
  switch(m_DisplayOption)
  {
  case CellWalls:
    this->DrawCellWalls();
    break;
  case RealNeighbors:
    this->DrawRealNeighbors();
    break;
  case Delaunay:
    this->DrawDelaunay();
    break;
  }
}


void
CellularAggregateViewer
::DrawCellWalls(void) const
{
  vtkSmartPointer< vtkPoints > newPoints = vtkPoints::New();

  MeshConstPointer mesh = this->m_CellularAggregate->GetMesh();
    
  const unsigned int numberOfPoints = mesh->GetNumberOfPoints();

  newPoints->Allocate( numberOfPoints );

  PointType position;

  CellsConstIterator cellIt = mesh->GetPointData()->Begin();
  CellsConstIterator end    = mesh->GetPointData()->End();

  PointsContainer::ConstPointer cellPositions = mesh->GetPoints();

  vtkIdType pointId = 0;
  while( cellIt != end )
    {
    BioCellType * cell = cellIt.Value();
    const IdentifierType id = cell->GetSelfIdentifier();
    if( cellPositions->GetElementIfIndexExists( id, &position ) )
      {
      newPoints->InsertPoint(pointId++,position[0],position[1],position[2]);
      }
    cellIt++;
    }

  this->m_PolyData->SetPoints(newPoints);

#if VTK_MAJOR_VERSION <= 5
  m_GlyphFilter->SetInput( this->m_PolyData );
  m_SurfaceMapper->SetInput( m_GlyphFilter->GetOutput() );
#else
  m_GlyphFilter->SetInputData( this->m_PolyData );
  m_GlyphFilter->Update();
  m_SurfaceMapper->SetInputData( m_GlyphFilter->GetOutput() );
#endif
}


void
CellularAggregateViewer
::DrawRealNeighbors(void) const
{
  
  MeshConstPointer mesh = this->m_CellularAggregate->GetMesh();

  vtkSmartPointer< vtkPoints > newPoints = vtkPoints::New();

  const unsigned int numberOfPoints = mesh->GetNumberOfPoints();

  newPoints->Allocate( numberOfPoints );

  PointType position;

  CellsConstIterator cellIt = mesh->GetPointData()->Begin();
  CellsConstIterator end    = mesh->GetPointData()->End();

  PointsContainer::ConstPointer cellPositions = mesh->GetPoints();

  typedef std::map< IdentifierType, vtkIdType > MapType;
  MapType pointIdMap;
    
  vtkIdType pointId = 0;
  while( cellIt != end )
    {
    BioCellType * cell = cellIt.Value();
    const IdentifierType id = cell->GetSelfIdentifier();
    if( cellPositions->GetElementIfIndexExists( id, &position ) )
      {
      pointIdMap[id] = pointId;
      newPoints->InsertPoint(pointId++,position[0],position[1],position[2]);
      }
    cellIt++;
    }

  this->m_PolyData->SetPoints(newPoints);

  // Adding now the triangle strips
  vtkSmartPointer< vtkCellArray > lines  = vtkCellArray::New();
  lines->Allocate( lines->EstimateSize( numberOfPoints, 20));

  vtkIdType pointIds[2];

  // Draw edges connecting neighbor cells
  cellIt = mesh->GetPointData()->Begin();
  while( cellIt != end )
    {
    BioCellType * cell = cellIt.Value();
    const IdentifierType id1 = cell->GetSelfIdentifier();

    pointIds[0] = pointIdMap[id1];

    VoronoiRegionAutoPointer voronoiRegion;
    m_CellularAggregate->GetVoronoi( id1, voronoiRegion );
              
    VoronoiRegionType::PointIdIterator neighbor = voronoiRegion->PointIdsBegin();
    VoronoiRegionType::PointIdIterator pend      = voronoiRegion->PointIdsEnd();

    while( neighbor != pend )
      {
      const IdentifierType id2 = (*neighbor);  
      
      if( !cellPositions->IndexExists( id2) )
        {
        ++neighbor;
        continue;// if the neigbor has been removed, skip it
        }
      else
        {
        pointIds[1] = pointIdMap[id2];
        lines->InsertNextCell( 2, pointIds );
        ++neighbor;
        }
      }
    cellIt++;
    }

  this->m_PolyData->SetLines( lines );

#if VTK_MAJOR_VERSION <= 5
  m_SurfaceMapper->SetInput( m_PolyData );
#else
  m_SurfaceMapper->SetInputData( m_PolyData );
#endif

}



void
CellularAggregateViewer
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf(os,indent);
  
  os << "Cellular aggregate " << m_CellularAggregate << std::endl;

}


void
CellularAggregateViewer
::DrawDelaunay(void) const
{
  vtkSmartPointer< vtkPoints > newPoints = vtkPoints::New();

  MeshConstPointer mesh = this->m_CellularAggregate->GetMesh();

  const unsigned int numberOfPoints = mesh->GetNumberOfPoints();

  newPoints->Allocate( numberOfPoints );

  PointType position;

  CellsConstIterator cellIt = mesh->GetPointData()->Begin();
  CellsConstIterator end    = mesh->GetPointData()->End();

  PointsContainer::ConstPointer cellPositions = mesh->GetPoints();

  vtkIdType pointId = 0;
  while( cellIt != end )
    {
    BioCellType * cell = cellIt.Value();
    const IdentifierType id = cell->GetSelfIdentifier();
    if( cellPositions->GetElementIfIndexExists( id, &position ) )
      {
      newPoints->InsertPoint(pointId++,position[0],position[1],position[2]);
      }
    cellIt++;
    }

  this->m_PolyData->SetPoints(newPoints);

  if( pointId > 3 )
    {
#if VTK_MAJOR_VERSION <= 5
    m_Delaunay2DFilter->SetInput( this->m_PolyData );
    m_SurfaceMapper->SetInput( m_Delaunay2DFilter->GetOutput() );
#else
    m_Delaunay2DFilter->SetInputData( this->m_PolyData );
    m_Delaunay2DFilter->Update();
    m_SurfaceMapper->SetInputData( m_Delaunay2DFilter->GetOutput() );
#endif
    }
  else
    {
#if VTK_MAJOR_VERSION <= 5
    m_SurfaceMapper->SetInput( this->m_PolyData );
#else
    m_SurfaceMapper->SetInputData( this->m_PolyData );
#endif
    }
}


void
CellularAggregateViewer
::SetRenderer(vtkRenderer * renderer)
{
  m_Renderer = renderer;
  m_Renderer->AddActor( m_SurfaceActor );
}



void
CellularAggregateViewer
::SetDisplayOption( DisplayOptionType option )
{
  m_DisplayOption = option;
}





} // end namespace bio

} // end namespace itk


