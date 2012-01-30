/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    Cell.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef Cell_H
#define Cell_H

#include <FL/fl_draw.H>
#include "fltkSphere3D.h"
#undef max
#include "itkRGBPixel.h"
#include "itkVector.h"
#include "itkPoint.h"
#include "Genome.h"

namespace bio {

class CellularAggregate;

/** \class Cell
 * \brief This class implement the minimal behavior 
 * of a biological cell.
 * The basic behavior of a cell is related with the
 * cell cycle. Geometrical concepts like size and shape
 * are also managed by this abstract cell.
 */
class Cell  
{
public:
#define CELL_DIMENSION 2
  enum { Dimension = CELL_DIMENSION };
  enum { PointDimension = Dimension };

  typedef   itk::Vector<double,PointDimension>  VectorType;
  typedef   itk::Point<double,PointDimension>   PointType;
  typedef   itk::RGBPixel<float>                ColorType;
  typedef   unsigned long                       IdentifierType;
  typedef   Genome::GeneIdType                  GeneIdType;

public:
  Cell();
  virtual ~Cell();
  virtual void Draw( const PointType & ) const;
  virtual void ClearForce(void);
  virtual void AddForce(const VectorType & force);
  virtual void AdvanceTimeStep(void);

  virtual void ComputeGeneNetwork(void);
  virtual void SecreteProducts(void);

  virtual void SetCellularAggregate( CellularAggregate * );

  virtual       CellularAggregate * GetCellularAggregate( void );
  virtual const CellularAggregate * GetCellularAggregate( void ) const;
  static  const char * GetSpeciesName(void) 
                              { return "Primitive Cell"; }
  static  Cell * CreateEgg(void);

  bool MarkedForRemoval(void) const;

protected:
  virtual void Grow(void);
  virtual void Mitosis(void);
  virtual void DNAReplication(void);
  virtual void Apoptosis(void);
  
  virtual void EnergyIntake(void);
  virtual void NutrientsIntake(void);
  virtual void ReceptorsReading(void);
  
  virtual bool CheckPointGrowth(void);
  virtual bool CheckPointDNAReplication(void);
  virtual bool CheckPointMitosis(void);
  virtual bool CheckPointApoptosis(void);

  virtual Cell * CreateNew(void);

  void MarkForRemoval(void);

  typedef enum {
                  M = 1UL,
                  Gap1,
                  S,
                  Gap2,
                  Gap0,
                  Apop,
                        } CellCycleState;

public:

  virtual const VectorType & GetForce(void) const;

  virtual ColorType GetColor(void) const;
  
  double GetRadius(void) const;
  
  IdentifierType GetSelfIdentifier(void) const;
  IdentifierType GetParentIdentifier(void) const;
  
  static void SetGrowthRadiusLimit( double );
  static void SetGrowthRadiusIncrement( double );
  static void SetEnergySelfRepairLevel( double );
  static void SetNutrientSelfRepairLevel( double );
  static void SetDefaultColor( const ColorType & color );

  static void SetGrowthMaximumLatencyTime( unsigned long latency );
  static unsigned long GetGrowthMaximumLatencyTime( void );

  static double GetGrowthRadiusLimit( void );
  static void SetMaximumGenerationLimit( unsigned long );
  
  static void ResetCounter(void);

protected:
   Genome             * m_Genome;
   Genome             * m_GenomeCopy;

   VectorType           m_Force;
   double               m_Pressure;

   ColorType            m_Color;
   
   double               m_Radius;
   double               m_EnergyReserveLevel;
   double               m_NutrientsReserveLevel;

   unsigned long        m_GrowthLatencyTime;

   IdentifierType       m_ParentIdentifier;
   IdentifierType       m_SelfIdentifier;

   unsigned long        m_Generation;

   CellCycleState       m_CycleState;

   CellularAggregate  * m_Aggregate;

   // Static Members
   static     double      DefaultRadius;
   static     ColorType   DefaultColor;

   static     GeneIdType  BlueGene;   // Pigment genes
   static     GeneIdType  RedGene;
   static     GeneIdType  GreenGene;
   static     GeneIdType  Cdk2E;      // cell cycle control  genes
   static     GeneIdType  Caspase;    // cleavage enzyme: apoptosis effector

   static     double      GrowthRadiusLimit;
   static     double      GrowthRadiusIncrement;

   static unsigned long   MaximumGenerationLimit;
   static unsigned long   GrowthMaximumLatencyTime;

   static     double      EnergySelfRepairLevel;
   static     double      NutrientSelfRepairLevel;

   static     double      DefaultEnergyIntake;
   static     double      DefaultNutrientsIntake;

   static     unsigned long  Counter;

   static     int         DisplayList;
     
   static fltk::Sphere3D::Pointer    SphereShape;

private:
   void DrawCircle(void) const;
   void DrawSphere(void) const;
   void DrawIcosaedron(void) const;

private:
   bool                       m_MarkedForRemoval;

};


} // end namespace bio

#endif
