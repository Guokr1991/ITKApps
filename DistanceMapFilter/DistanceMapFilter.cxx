


#include "DistanceMapFilter.h"
#include <FL/Fl_File_Chooser.H>
#include "FL/fl_ask.H"


DistanceMapFilter::
DistanceMapFilter()
{
}


DistanceMapFilter::
~DistanceMapFilter()
{
}


void
DistanceMapFilter::
ComputeDistance(void)
{
   
  if( ! this->ImageHasBeenLoaded() )
    {
    fl_alert("Please load an image first");
    return;
    }

  progressWindow->show();
  Fl::check();
  DistanceMapFilterBase::ComputeDistance();
  progressWindow->hide();
}



  

