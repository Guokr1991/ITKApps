/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    CellularAggregateViewer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __Cellular_Aggregate_Viewer_H
#define __Cellular_Aggregate_Viewer_H

// Disable warning for long symbol names in this file only
#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif


#include <iostream>
#include "itkObject.h"
#include "itkObjectFactory.h"
#include "itkBioCellularAggregate.h"

class vtkPolyData;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkActor;
class vtkDelaunay2D;
class vtkGlyph2D;
class vtkCylinderSource;
class vtkTransform;
class vtkTransformPolyDataFilter;

namespace itk {

namespace bio {

/** \class CellularAggregateViewer
 * \brief This class is intended to display a Cellular Aggregate in different
 * ways using VTK.
 */
class CellularAggregateViewer : public itk::Object
{
public:
  /** Standard class typedefs. */
  typedef CellularAggregateViewer      Self;
  typedef itk::Object  Superclass;
  typedef itk::SmartPointer<Self>        Pointer;
  typedef itk::SmartPointer<const Self>  ConstPointer;

  /*** Run-time type information (and related methods). */
  itkTypeMacro(CellularAggregateViewer, itk::Object);

  /** Method for creation through the object factory. */
  itkNewMacro(Self);  

  typedef enum 
    {
    CellWalls,
    RealNeighbors,
    Delaunay
    } DisplayOptionType;


  void SetDisplayOption( DisplayOptionType );

  unsigned int GetNumberOfCells(void) const;
 
  void Draw(void) const;

  void SetRenderer( vtkRenderer * );

  typedef ::itk::bio::CellularAggregate<2>                  CellularAggregateType;
  typedef CellularAggregateType::ConstPointer               CellularAggregateConstPointer;
  typedef CellularAggregateType::MeshPointer                MeshPointer;
  typedef CellularAggregateType::MeshConstPointer           MeshConstPointer;
  typedef CellularAggregateType::PointType                  PointType;
  typedef CellularAggregateType::CellsIterator              CellsIterator;
  typedef CellularAggregateType::CellsConstIterator         CellsConstIterator;
  typedef CellularAggregateType::PointsContainer            PointsContainer;
  typedef CellularAggregateType::IdentifierType             IdentifierType;
  typedef CellularAggregateType::VoronoiRegionType          VoronoiRegionType;
  typedef CellularAggregateType::VoronoiRegionAutoPointer   VoronoiRegionAutoPointer;
  typedef CellularAggregateType::BioCellType                BioCellType;


  itkSetConstObjectMacro(CellularAggregate,CellularAggregateType);
  itkGetConstObjectMacro(CellularAggregate,CellularAggregateType);
  
protected:
  CellularAggregateViewer();
  virtual ~CellularAggregateViewer();
  void PrintSelf(std::ostream& os, itk::Indent indent) const;

  virtual void DrawCellWalls(void) const;
  virtual void DrawRealNeighbors(void) const;
  virtual void DrawDelaunay(void) const;

private:
  CellularAggregateConstPointer   m_CellularAggregate;

  vtkPolyData          *m_PolyData;
  vtkRenderer          *m_Renderer;
  vtkPolyDataMapper    *m_SurfaceMapper;
  vtkActor             *m_SurfaceActor;
  vtkDelaunay2D        *m_Delaunay2DFilter;
  vtkGlyph2D           *m_GlyphFilter;
  vtkCylinderSource    *m_CellGlyphSource;
  vtkTransform         *m_CellGlyphTransform;       
  vtkTransformPolyDataFilter *m_CellGlyphTransformFilter; 

  DisplayOptionType     m_DisplayOption;
};

} // end namespace bio

} // end namespace itk

#endif

