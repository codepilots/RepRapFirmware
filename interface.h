/*
 * interface.h
 *
 *  Created on: 9 Aug 2014
 *      Author: chris
 */

#ifndef INTERFACE_H_
#define INTERFACE_H_


//
// interface.h
// From: http://www.codeguru.com/cpp/cpp/cpp_mfc/oop/article.php/c9989/Using-Interfaces-in-C.htm


#define Interface class

#define DeclareInterface(name) Interface name { \
          public: \
          virtual ~name() {}

#define DeclareBasedInterface(name, base) class name : \
        public base { \
           public: \
           virtual ~name() {}

#define EndInterface };

#define implements public


#endif /* INTERFACE_H_ */
