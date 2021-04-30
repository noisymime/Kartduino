#include "page_printascii.h"
#include "globals.h"
#include "table_iterator.h"

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

// Alias page 2 - it's page 1 in the INI file
#define configPage1 configPage2

// Pull in the generated code.
#include "page_printascii.g.hpp"