//! @file
#pragma once

//! Printer variant
//!@{

#define PRINTER_PRUSA_MK3         1 //!< MK3 printer
#define PRINTER_PRUSA_MINI        2 //!< MINI printer
#define PRINTER_PRUSA_XL          3 //!< XL printer
#define PRINTER_PRUSA_MK4         4 //!< MK3 printer with new extruder
#define PRINTER_PRUSA_IXL         5 //!< iXL printer
#define PRINTER_PRUSA_MANIPULATOR 6
#define PRINTER_PRUSA_PICKER      7
#define PRINTER_PRUSA_EXTRACTOR   8

//!@}

#ifndef PRINTER_TYPE
    #error "macro PRINTER_TYPE not defined"
#endif
