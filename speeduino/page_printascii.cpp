#include "page_printascii.h"
#include "globals.h"
#include "table_iterator.h"
#include "utilities.h"

/// Prints each element in the memory byte range (*first, *last).
template <typename _Element>
static void serial_println_range(Print &target, const _Element *first, const _Element *last)
{
    while (first!=last)
    {
        target.println(*first);
        ++first;
    }
}

template <typename _Element>
static void serial_print_space_delimited(Print &target, const _Element *first, const _Element *last)
{
    while (first!=last)
    {
        target.print(*first);// This displays the values horizantially on the screen
        target.print(F(" "));
        ++first;
    }
    target.println();
}

static void serial_print_prepadding(Print &target, byte value)
{
    if (value < 100)
    {
        target.print(F(" "));
        if (value < 10)
        {
            target.print(F(" "));
        }
    }
}

static void serial_print_prepadded_value(Print &target, byte value)
{
    serial_print_prepadding(target, value);
    target.print(value);
    target.print(F(" "));
}

static void print_row(Print &target, const table_axis_iterator_t &y_it, table_row_t row)
{
    serial_print_prepadded_value(target, get_value(y_it));

    while (!at_end(row))
    {
        serial_print_prepadded_value(target, *row.pValue++);
    }
    target.println();
}

static void print_x_axis(Print &target, table_axis_iterator_t x_it)
{
    target.print(F("    "));

    while(!at_end(x_it))
    {
        serial_print_prepadded_value(target, get_value(x_it));
        advance_axis(x_it);
    }
}

static void serial_print_3dtable(Print &target, table_row_iterator_t row_it, table_axis_iterator_t x_it, table_axis_iterator_t y_it)
{
    while (!at_end(row_it))
    {
        print_row(target, y_it, get_row(row_it));
        advance_axis(y_it);
        advance_row(row_it);
    }

    print_x_axis(target, x_it);
    target.println();
}

static void serial_print_3dtable(Print &target, const table3D &currentTable)
{
    serial_print_3dtable(target, rows_begin(&currentTable), x_begin(&currentTable), y_begin(&currentTable));
}

#define print_array(outputName, array) serial_print_space_delimited(outputName, array, array+_countof(array));

// Alias page 2 - it's page 1 in the INI file
#define configPage1 configPage2
// As per INI comment
// ;Has to be called algorithm for the req fuel calculator to work :(
#define algorithm fuelAlgorithm
// These are stored in an array of structs
#define firstCompType0 operation[0].firstCompType
#define secondCompType0 operation[0].secondCompType
#define bitwise0 operation[0].bitwise
#define firstCompType1 operation[1].firstCompType
#define secondCompType1 operation[1].secondCompType
#define bitwise1 operation[1].bitwise
#define firstCompType2 operation[2].firstCompType
#define secondCompType2 operation[2].secondCompType
#define bitwise2 operation[2].bitwise
#define firstCompType3 operation[3].firstCompType
#define secondCompType3 operation[3].secondCompType
#define bitwise3 operation[3].bitwise
#define firstCompType4 operation[4].firstCompType
#define secondCompType4 operation[4].secondCompType
#define bitwise4 operation[4].bitwise
#define firstCompType5 operation[5].firstCompType
#define secondCompType5 operation[5].secondCompType
#define bitwise5 operation[5].bitwise
#define firstCompType6 operation[6].firstCompType
#define secondCompType6 operation[6].secondCompType
#define bitwise6 operation[6].bitwise
#define firstCompType7 operation[7].firstCompType
#define secondCompType7 operation[7].secondCompType
#define bitwise7 operation[7].bitwise

// Pull in the generated code.
#include "page_printascii.g.hpp"