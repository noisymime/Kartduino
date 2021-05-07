import re
import more_itertools
import itertools

from itertools import groupby

_regexComma = r'(?:\s*,\s*)'
_keyRegEx = r'^\s*(?P<key>[^;].+)\s*'
_dataTypeRegex = fr'{_regexComma}(?P<type>.\d+)'
_fieldOffsetRegex = fr'{_regexComma}(?P<offset>\d+)'
_otherRegEx = fr'({_regexComma}(?P<other>.+))*'

class IniComment:
    regex = re.compile(fr'^\s*(;.*)$')

    def __init__(self, match):
        self.Comment = match.group(1).strip()

    def __str__(self):
        return self.Comment

class IniSection:
    regex = re.compile(fr'^\s*\[(.+)\]\s*$')
        
    def __init__(self, match):
        self.Section = match.group(1).strip()

    def __str__(self):
        return self.Section

class IniField:
    __types = {
        'S08' : [ 'int8_t', 1 ],
        'S16' : [ 'int16_t', 2 ],
        'U08' : [ 'uint8_t', 1 ],
        'U16' : [ 'uint16_t', 2 ],
    }

    def __init__(self, match):
        self.Field = match.group('key').strip()
        self.DataType = IniField.__types[match.group('type').strip()]
        self.Offset = int(match.group('offset') or -1)
        self.Other = [x.strip() for x in (match.group('other') or "").split(',')]

class IniScalarField(IniField):
    regex = re.compile(fr'{_keyRegEx}=(?:\s*scalar){_dataTypeRegex}(?:{_fieldOffsetRegex})?{_otherRegEx}')

    def __init__(self, match):
        super().__init__(match)
        self.OffsetEnd = self.Offset

    def __str__(self):
        return f'{self.Field}=scalar,{self.DataType[0]}'
        
class IniBitField(IniField):
    regex = re.compile(fr'{_keyRegEx}=(?:\s*bits){_dataTypeRegex}(?:{_fieldOffsetRegex})?{_regexComma}(?:\[(?P<bitStart>\d+):(?P<bitEnd>\d+)\]){_otherRegEx}')

    def __init__(self, match):
        super().__init__(match)
        self.BitStart = int(match.group('bitStart'))
        self.BitEnd = int(match.group('bitEnd'))
        self.OffsetEnd = self.Offset

    def __str__(self):
        return f'{self.Field}=bit,{self.DataType[0]},[{self.BitStart}:{self.BitEnd}]'

class IniTwoDimArrayField(IniField):
    regex = re.compile(fr'{_keyRegEx}=(?:\s*array){_dataTypeRegex}(?:{_fieldOffsetRegex})?{_regexComma}(?:\[\s*(?P<xDim>\d+)x(?P<yDim>\d+)\s*\]){_otherRegEx}')

    def __init__(self, match):
        super().__init__(match)
        self.xDim = int(match.group('xDim'))
        self.yDim = int(match.group('yDim'))
        self.OffsetEnd = self.Offset + (self.xDim * self.yDim * self.DataType[1])

    def __str__(self):
        return f'{self.Field}=2d-array,{self.DataType[0]},[{self.xDim}x{self.yDim}]'

class IniOneDimArrayField(IniField):
    regex = re.compile(fr'{_keyRegEx}=(?:\s*array){_dataTypeRegex}(?:{_fieldOffsetRegex})?{_regexComma}(?:\[\s*(?P<length>\d+)\s*\]){_otherRegEx}')

    def __init__(self, match):
        super().__init__(match)
        self.Length = int(match.group('length'))
        self.OffsetEnd = self.Offset + ((self.Length-1) * self.DataType[1])

    def __str__(self):
        return f'{self.Field}=array,{self.DataType[0]},[{self.Length}]'

class IniString:
    regex = re.compile(fr'{_keyRegEx}=(?:\s*string){_regexComma}(?P<encoding>.+){_regexComma}(?:\s*(?P<length>\d+))')

    def __init__(self, match):
        self.Field = match.group('key').strip()
        self.Encoding = match.group('encoding').strip()
        self.Length = int(match.group('length'))

    def __str__(self):
        return f'{self.Field}=string,[{self.Length}]'

class IniKeyValue:
    regex = re.compile(fr'{_keyRegEx}=\s*(?P<value>.*)\s*$')

    def __init__(self, match):
        self.Key = match.group('key').strip()
        self.Values = [x.strip() for x in (match.group('value') or "").split(',')]

    def __str__(self):
        return f'{self.Key}={self.Values}'

class IniDefine:
    regex = re.compile(fr'^\s*#define\s+(?P<condition>.+)\s*=\s*(?P<value>.+)\s*$')

    def __init__(self, match):
        self.Condition = match.group('condition').strip()
        self.Value = match.group('value').strip()

    def __str__(self):
        return f'{self.Condition}={self.Value}'

class IniBeginIfdef:
    regex = re.compile(fr'^\s*#if\s+(?P<condition>.*)\s*$')

    def __init__(self, match):
        self.Condition = match.group('condition').strip()

    def __str__(self):
        return f'#if {self.Condition}' 

class IniIfDef:
    def __init__(self, condition, iflines, elselines):
        self.Condition = condition
        self.IfLines = iflines
        self.ElseLines = elselines

class IniIfDefElse:
    regex = re.compile(fr'^\s*#else\s*$')

    def __init__(self, match):
        pass

    def __str__(self):
        return f'#else'         

class IniEndIfdef:
    regex = re.compile(fr'^\s*#endif\s*$')

    def __init__(self, match):
        pass

    def __str__(self):
        return f'#endif' 

class UnknownLine:
    def __init__(self, line):
        self.Line = line

    def __str__(self):
        return self.Line

def read_tsini(iniFile):
    def process_line(line):
        ts_ini_regex_handlers = [
            IniComment,
            IniSection,
            IniScalarField,
            IniBitField,
            IniTwoDimArrayField,
            IniOneDimArrayField,
            IniString,
            IniDefine,
            IniBeginIfdef,
            IniIfDefElse,
            IniEndIfdef,
            IniKeyValue
        ]
        for line_type in ts_ini_regex_handlers:
            match = re.match(line_type.regex, line)
            if match:
                return line_type(match)

        return UnknownLine(line)

    def coalesce_ifdefs(group):
        def coalesce_ifdef(iterable):
            ifdef = more_itertools.first(group)
            iflines, *elselines = more_itertools.split_at(
                            more_itertools.islice_extended(group, 1, -1), 
                            lambda item: isinstance(item, IniIfDefElse))
            return [IniIfDef(ifdef.Condition, iflines, elselines[0] if elselines else [])]

        if isinstance(more_itertools.first(group), IniBeginIfdef):
            return coalesce_ifdef(group)
        return group

    # Load and convert
    with open(iniFile, 'r') as f:
        lines = [process_line(x) for x in f if None!=x and not str.isspace(x)]
    if not isinstance(lines[0], IniSection):
        lines.insert(0, process_line('[None]'))

    # Collapse #if groups into one item
    ifGroups = more_itertools.collapse(
                map(coalesce_ifdefs, 
                    more_itertools.split_when(lines, 
                        lambda itemA, itemB: isinstance(itemB, IniBeginIfdef) or isinstance(itemA, IniEndIfdef)
                )))

    # Group into sections
    groups = more_itertools.split_before(ifGroups, lambda item: isinstance(item, IniSection))
    return { item[0].Section: item[1:] for item in groups}


# The INI file allows the addresses of fields to overlap - even for scalar and array fields
# (bit fields obviously overlap)
# 
# This function groups those overlapping fields
def group_overlapping(fields):
    def is_overlapping(field1, field2):
        def overlap_helper(field1, field2):
            return (field1.Offset >= field2.Offset) and (field1.Offset <= field2.OffsetEnd)
        return overlap_helper(field1, field2) or overlap_helper(field2, field1)

    class group_overlap:       
        def __init__(self):
            self.group_start = None

        def __call__(self, item):
            if not self.group_start:
                self.group_start = item
            # We order by widest item first at any overlapping address
            # So we must compare subsequent items to that first item in the group.
            if is_overlapping(self.group_start, item):
                return False
            # No overlaap - start of new group
            else:
                self.group_start = item
                return True

    return more_itertools.split_before(
                # The sorting here is critical for the group_overlap algorithm
                # Sort by start address ascending, then end address descending. I.e. for
                # any overlapping fields, put the widest field first
                #
                # We assume overlaps are distinct and complete. I.e.
                #  0-7, 6-9 is not allowed in the INI file
                sorted(fields, key=lambda item: (item.Offset, -item.OffsetEnd)), 
                group_overlap())
