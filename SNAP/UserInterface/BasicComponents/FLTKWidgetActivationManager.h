/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    FLTKWidgetActivationManager.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef _FLTKWidgetActivationManager_h_
#define _FLTKWidgetActivationManager_h_

#include "FL/Fl_Widget.H"
#include "FL/Fl_Menu_Item.H"
#include "FL/Fl_Check_Button.H"
#include <map>
#include <set>
#include <list>

using namespace std;

/**
 * \class WidgetActivationManager 
 * \brief A generic state machine for widget management
 * \see FLTKWidgetActivationManager
 */
template<typename TFlag>
class WidgetActivationManager {
public:

  /** 
   * With default parameters, this call specifies that flag A implies flag B.
   * Using the optional parameters, you can specify A->!B, !A->B or !A->!B.
   * Read: A in stateA implies B is in stateB 
   * */
  void SetFlagImplies(TFlag A, TFlag B, bool stateA=true, bool stateB=true)
    {
    // Apply the forward rule
    m_Flags[A].Rules[ stateA ? 1 : 0 ].push_back( Rule(B, stateB) );

    // Apply the reverse rule
    m_Flags[B].Rules[ stateB ? 0 : 1 ].push_back( Rule(A, !stateA) );
    }

  /** 
   * This is a quick way to set up a set of flags that are mutually exclusive
   * (but all the flags may be set to false at once)
   */
  void SetFlagsMutuallyExclusive( int nFlags, TFlag *F )
    {
    // For each flag, set up a rule to all the other flags
    for(unsigned int i = 0; i < nFlags; i++) 
      for(unsigned int j = 0; j < i; j++)
        SetFlagImplies( F[i], F[j], true, false );
    }

  /** Set up mutual exclusion between three flags */
  void SetFlagsMutuallyExclusive( TFlag A, TFlag B, TFlag C )
    {
    SetFlagImplies( A, B, true, false );
    SetFlagImplies( A, C, true, false );
    SetFlagImplies( B, C, true, false );
    }

  /** 
   * Update a flag to a new value. Ignore the third parameter, it
   * is used internally to throw an exception if the flag machine
   * enters an infinite loop
   */
  void UpdateFlag(TFlag flag, bool value, int recursionCount = 0)
    {
    // Do nothing if the flag already has the given value
    if(m_Flags[flag].State == value) return;

    // Throw an exception if the recursion depth is too great
    if(recursionCount > m_Flags.size())
      throw std::string("Infinite loop in the flag state machine!");

    // Get the flag data for this flag
    FlagData &fd = m_Flags[flag];

    // Update the flag
    fd.State = value;

    // Trigger all the rules for this flag. We do this before updating the widgets 
    // so that the settings related to the actual flag being updated are applied last
    typename list<Rule>::iterator itRule = fd.Rules[ value ? 1 : 0 ].begin();   
    while( itRule != fd.Rules[ value ? 1 : 0 ].end() )
      {
      UpdateFlag( itRule->Target, itRule->Value, recursionCount + 1 );
      ++itRule;
      }

    // Activate all the widgets tied to this flag
    typename set<WidgetWrapper *>::iterator itWidget = fd.Widgets.begin();    
    while( itWidget != fd.Widgets.end() )
      (*itWidget++)->OnStateChange(value);
        
    }

  /** Destructor, deletes widget wrappers */
  virtual ~WidgetActivationManager()
    {
    typename set<WidgetWrapper *>::iterator itWidget = m_AllWidgets.begin();
    while(itWidget != m_AllWidgets.end())
      delete *(itWidget++);
    }

  /**
   * A class used to control the state of an FLTK widget
   */
  class WidgetWrapper {
  public:
    virtual void OnStateChange(bool newState) = 0;
  };

protected:

  /** 
   * A rule relating one flag to another. The rule has a target and a
   * value. When a rule is triggered, it set's the target flag to the value
   */
  struct Rule {
    // The target of the rule
    TFlag Target;

    // The value the rule assigns to the target
    bool Value;

    // A constructor
    Rule(TFlag target, bool value) : Target(target), Value(value) {}
  };

  /**
   * A structure associated with a flag
   */ 
  struct FlagData
    {
    // A list of rules that are triggered when the flag is set to true
    // and false states
    list<Rule> Rules[2];

    // A list of widget controllers that are affected by the flag's state
    set<WidgetWrapper *> Widgets;

    // The state of the menu item
    bool State;

    // Constructor
    FlagData() { State = false; }
    };

  /**
   * Add a controlled widget to a flag. The wrapper will be deleted when
   * the destructor is called, so don't call delete!
   */
  void AddWidgetWrapper(WidgetWrapper *widget, TFlag flag)
    {
    m_Flags[flag].Widgets.insert(widget);
    m_AllWidgets.insert(widget);
    widget->OnStateChange(false);
    }

private:
  // A list of flags and associated flag data
  map<TFlag, FlagData> m_Flags;

  // A list of widget wrappers, to be deleted
  set<WidgetWrapper *> m_AllWidgets;
};

/**
 * \class FLWidgetActivationManager
 * \brief Simplifies management of widget on/off status
 * This class can link the on-off status of FLTK widgets and menu items
 * with a set of flags in a 'state machine'. The flags can be related to
 * each other, for example, a flag can imply another flag or a set of
 * flags can be mutually exclusive
 */
template<typename TFlag>
class FLTKWidgetActivationManager : public WidgetActivationManager<TFlag>
{
public:
  /** Associate an FLTK widget with a flag */
  void AddWidget(Fl_Widget *widget, TFlag flag)
    {
    typedef GenericWidgetWrapper<Fl_Widget> WrapperType;
    AddWidgetWrapper(new WrapperType(widget), flag);
    }

  /** Associate an FLTK menu item with a flag */
  void AddMenuItem(Fl_Menu_Item *menu, TFlag flag)
    {
    typedef GenericWidgetWrapper<Fl_Menu_Item> WrapperType;
    AddWidgetWrapper(new WrapperType(menu), flag);
    }

  /** Associate an FLTK checkbox with a flag and two values */
  void AddCheckBox(Fl_Check_Button *cb, TFlag flag, bool onState, bool offState)
    {
    typedef ValuatorWidgetWrapper<Fl_Check_Button, int> WrapperType;
    AddWidgetWrapper(new WrapperType(cb,onState ? 1 : 0,offState ? 1 : 0), flag);    
    }

protected:
  
  /** 
   * A wrapper for a generic FLTK widget or menu item, calls activate
   * or deactivate depending on the state 
   */
  template<class TWidget>
    class GenericWidgetWrapper : public WidgetActivationManager<TFlag>::WidgetWrapper {
  public:
    
    // State change method
    virtual void OnStateChange(bool newState) 
      {
      if(newState) m_Widget->activate();
      else m_Widget->deactivate();
      }
    
    // Constructor
    GenericWidgetWrapper(TWidget *w) : m_Widget(w) {};

  protected:
    TWidget *m_Widget;
  };

  /** 
   * A special wrapper for FLTK widgets that can be assigned a value. It
   * can assign a default value to the widget when it's deactivated and when
   * it's activated
   */
  template<class TWidget, class TValue>
  class ValuatorWidgetWrapper
  : public GenericWidgetWrapper<TWidget> {
  public:
    // State change method
    virtual void OnStateChange(bool newState)
      {
      GenericWidgetWrapper<TWidget>::OnStateChange(newState);
      m_Widget->value( newState ? m_OnValue : m_OffValue );
      }

    // Constructor
    ValuatorWidgetWrapper(TWidget *w, const TValue &on, const TValue &off)
      : GenericWidgetWrapper<TWidget>(w), m_OnValue(on), m_OffValue(off) {}

  protected:
    TValue m_OffValue, m_OnValue;
  };
  
};





#endif
