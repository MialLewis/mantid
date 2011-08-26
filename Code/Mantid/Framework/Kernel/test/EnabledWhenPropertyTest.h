#ifndef MANTID_KERNEL_ENABLEDWHENPROPERTYTEST_H_
#define MANTID_KERNEL_ENABLEDWHENPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerOwner.h"
#include "MantidKernel/Property.h"

using namespace Mantid;
using namespace Mantid::Kernel;

class EnabledWhenPropertyTest : public CxxTest::TestSuite
{
public:

  void test_when_IS_NOT_DEFAULT()
  {
    PropertyManagerOwner alg;
    // Start with a regular property
    alg.declareProperty("MyIntProp", 123);

    // Make a property with its validator. Will be enabled when that other one is NOT the default
    EnabledWhenProperty<int> * val = new EnabledWhenProperty<int>(&alg, "MyIntProp", IS_NOT_DEFAULT);
    alg.declareProperty("MyValidatorProp", 456, val);

    Property * prop = alg.getPointerToProperty("MyValidatorProp");
    TS_ASSERT(prop); if (!prop) return;

    TSM_ASSERT( "Starts off NOT enabled",  !prop->isEnabled());
    alg.setProperty("MyIntProp", 234);
    TSM_ASSERT( "Becomes enabled when another property has been changed",  prop->isEnabled());

    IValidator<int> * val2 = val->clone();
    alg.declareProperty("MySecondValidatorProp", 456, val2);
    prop = alg.getPointerToProperty("MySecondValidatorProp");
    TSM_ASSERT( "Starts off enabled",  prop->isEnabled());
    alg.setProperty("MyIntProp", 123);
    TSM_ASSERT( "Goes back to disabled", !prop->isEnabled());
  }

  void test_when_IS_DEFAULT()
  {
    PropertyManagerOwner alg;
    alg.declareProperty("MyIntProp", 123);
    // Make a property with its validator. Will be enabled when that other one is the default
    EnabledWhenProperty<int> * val = new EnabledWhenProperty<int>(&alg, "MyIntProp", IS_DEFAULT);
    alg.declareProperty("MyValidatorProp", 456, val);
    Property * prop = alg.getPointerToProperty("MyValidatorProp");
    TS_ASSERT(prop); if (!prop) return;
    TSM_ASSERT( "Starts off enabled", prop->isEnabled());
    alg.setProperty("MyIntProp", -1);
    TSM_ASSERT( "Becomes disabled when another property has been changed", !prop->isEnabled());
  }

  void test_when_IS_EQUAL_TO()
  {
    PropertyManagerOwner alg;
    alg.declareProperty("MyIntProp", 123);
    EnabledWhenProperty<int> * val = new EnabledWhenProperty<int>(&alg, "MyIntProp", IS_EQUAL_TO, "234");
    alg.declareProperty("MyValidatorProp", 456, val);
    Property * prop = alg.getPointerToProperty("MyValidatorProp");
    TS_ASSERT(prop); if (!prop) return;
    TSM_ASSERT( "Starts off disabled", !prop->isEnabled());
    alg.setProperty("MyIntProp", 234);
    TSM_ASSERT( "Becomes enabled when the other property is equal to the given string", prop->isEnabled());
  }

  void test_when_IS_NOT_EQUAL_TO()
  {
    PropertyManagerOwner alg;
    alg.declareProperty("MyIntProp", 123);
    EnabledWhenProperty<int> * val = new EnabledWhenProperty<int>(&alg, "MyIntProp", IS_NOT_EQUAL_TO, "234");
    alg.declareProperty("MyValidatorProp", 456, val);
    Property * prop = alg.getPointerToProperty("MyValidatorProp");
    TS_ASSERT(prop); if (!prop) return;
    TSM_ASSERT( "Starts off enabled", prop->isEnabled());
    alg.setProperty("MyIntProp", 234);
    TSM_ASSERT( "Becomes disabled when the other property is equal to the given string", !prop->isEnabled());
  }


};


#endif /* MANTID_KERNEL_ENABLEDWHENPROPERTYTEST_H_ */

