/*
 * IDisplayType.h
 *
 *  Created on: 9 Aug 2014
 *      Author: chris
 */

#ifndef IDISPLAYTYPE_H_
#define IDISPLAYTYPE_H_

#include "../interface.h"

DeclareInterface(IDisplayType)
   virtual int GetBarData() const = 0;
   virtual void SetBarData(int nData) = 0;
EndInterface


#endif /* IDISPLAYTYPE_H_ */
