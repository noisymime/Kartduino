# Speeduino: a python script to generate code to print each TS page as ASCII text.
#
# Assumptions:
# 1. Each non-table page is declared as a struct named 'configPageN'. E.g. configPage3, configPage13
# 2. Each 3D table is declared using the same name as the corresponding table identifier in the INI file.
# 
# The generated code is not stand alone. It relies on supporting code defined in the Speeduino codebase
#
# Example usage: py .\gen-printpageascii.py > ..\speeduino\page_printascii.g.hpp

from read_tsini import *
import os
import itertools
import more_itertools

def take_if(item):
    if isinstance(item, IniIfDef):
        return item.IfLines
    else:
        return item

# Code printing

OUTPUT_VAR_NAME = 'target'

def out_fields(page_num, fields):
    def get_fullfieldname(page_num, field):
        return f'configPage{page_num}.{field.Field}'

    def out_scalar(page_num, field):
        print(f'\t{OUTPUT_VAR_NAME}.println({get_fullfieldname(page_num, field)});')

    def out_bit(page_num, field):
        print(f'\t{OUTPUT_VAR_NAME}.println({get_fullfieldname(page_num, field)});')

    def out_array(page_num, field):
        print(f'\tprint_array({OUTPUT_VAR_NAME}, {get_fullfieldname(page_num, field)});')
    
    print_map = {
        IniScalarField : out_scalar,
        IniBitField : out_bit,
        IniOneDimArrayField: out_array
    }

    for field in fields: 
        printer = print_map.get(type(field), None)
        if printer:
            printer(page_num, field)

def out_tables(tables):
    for table in tables: 
        tableName = table[0].Values[2].replace('"', '')
        print(f'\t{OUTPUT_VAR_NAME}.println(F("\\n{tableName}"));')
        print(f'\tserial_print_3dtable({OUTPUT_VAR_NAME}, {table[0].Values[0]});')   

def get_printpagefunctionname(page_num):
    return f'printPage{page_num}'

def out_printpageascii(page_num, fields):
    # TS appear to use a "last one wins" algorithm. 
    # This is useful for us - we use a different algorithm and can therefore
    # define fields in the INI file that TS will not use. 
    def flatten_field_group(field_group):
        non_bits = [item for item in field_group if not isinstance(item, IniBitField)]        
        if non_bits:
            # Pick the widest non-bit field
            return max(non_bits, key=lambda item: item.OffsetEnd-item.Offset)
        # Assume groups that are all bit fields do not really overlap
        return field_group

    print(f'static void {get_printpagefunctionname(page_num)}(Print &{OUTPUT_VAR_NAME}) {{')
    print(f'\t{OUTPUT_VAR_NAME}.println(F("\\nPg {page_num} Cfg"));')

    # Deal with overlapping fields
    print_fields = more_itertools.collapse(
                    map(flatten_field_group, 
                        group_overlapping(
                            (field for field in fields if not field.Table)
                        )))
    out_fields(page_num, print_fields)
    
    # Each table in the INI file is at least 3 entries - we only need one
    tables = itertools.groupby((field for field in fields if field.Table), 
                                key = lambda item: item.Table)
    out_tables(tables)

    print('}') 
    print('')  

def main():
    def is_page(item):
        return isinstance(item, IniKeyValue) and item.Key == 'page'

    def is_table(item):
        return isinstance(item, IniKeyValue) and item.Key == 'table'
    
    def is_comment(item):
        return isinstance(item, IniComment)

    def is_field(item):
        return isinstance(item, IniField)

    def find_table(tables, field):
        if is_field(field):
            for key, items in tables.items():
                if any(item.Values[0]==field.Field for item in items):
                    return key
        return None

    # Basic TS INI parse
    currDir = os.path.dirname(os.path.abspath(__file__))
    iniFile = os.path.join(currDir, '..', 'reference', 'speeduino.ini')
    lines = read_tsini(iniFile)

    # Gather 3D table information - required later   
    tables = (item for item in lines["TableEditor"] if not is_comment(item))
    # Group into pages
    tables = { table[0]: table[1:] for table in 
                (group for group in 
                    more_itertools.split_before(tables, is_table))}

    # Find all non-comment lines in the "Constants" section & take one side of any #if 
    page_lines = more_itertools.collapse((take_if(item) for item in lines["Constants"] if not is_comment(item)))
    # Tag each line with it's table
    field_table_setter = lambda item: setattr(item, 'Table', find_table(tables, item)) or item
    page_lines = (field_table_setter(item) for item in page_lines)
    # Group into pages
    pages = { page[0].Values[0]: page[1:] for page in 
                (group for group in 
                    more_itertools.split_before(page_lines, is_page)
                if is_page(group[0]))}

    print('/*')
    print('DO NOT EDIT THIS FILE.')
    print('')
    print('It is auto generated and your edits will be overwritten')
    print('*/')
    print('')

    for page in pages.items():
        out_printpageascii(*page)

    print(f'void printPageAscii(byte pageNum, Print &{OUTPUT_VAR_NAME}) {{')
    print('\tswitch(pageNum) {')
    for page in pages.items():
        print(f'\t\tcase {page[0]}:')
        print(f'\t\t{get_printpagefunctionname(page[0])}({OUTPUT_VAR_NAME});')
        print(f'\t\tbreak;')
    print('\t}') 
    print('}')

if __name__ == "__main__":
    main()