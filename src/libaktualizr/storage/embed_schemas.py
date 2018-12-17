#!/usr/bin/env python3

import sys
import os
import re

warning_text = '''
//This is autogenerated file, don't modify it manually
//Latest schema timestamp: %d
#include <string>
#include <vector>
'''

def escape_string(sql):
    return sql.translate(str.maketrans({'"':  r'\"', '\n':  '\\n'}))


def apend_migration(migration_path, header_file):
    migration_file = open(migration_path, 'r')
    header_file.write("\"")
    migration_content = migration_file.read()
    header_file.write(escape_string(migration_content))


if __name__ == '__main__':
    if len(sys.argv) != 4:
        print("\nIncorrect arguments")
        print("Usage:\n {} {} {} {}\n".format(sys.argv[0], "folder_with_schemas", "generated_file_path", "name_prefix"))
        sys.exit(-1)

    sql_folder = sys.argv[1]
    schemas_header = sys.argv[2]
    prefix = sys.argv[3]
    migration_folder = os.path.join(sql_folder, 'migration')
    migration_list = sorted(os.listdir(migration_folder))

    file_depends_list = [os.path.join(migration_folder, p) for p in migration_list]
    file_depends_list.append(__file__)  # set dependency on itself (this script)
    max_file_stamp = max(os.path.getmtime(p) for p in file_depends_list)

    pattern = re.compile("//Latest schema timestamp: (\d+)")
    if os.path.exists(schemas_header):
        for i, line in enumerate(open(schemas_header)):
            for match in re.finditer(pattern, line):
                if int(match.groups()[0]) == int(max_file_stamp):
                    # header up to date, exiting
                    sys.exit(0)

    with open(schemas_header, 'w') as header_file:
        header_file.write(warning_text % max_file_stamp)
        header_file.write("extern const std::vector<std::string> {}schema_migrations = {{".format(prefix))
        for migration in migration_list[:-1]:
            apend_migration(os.path.join(migration_folder, migration), header_file)
            header_file.write("\",\n")
        apend_migration(os.path.join(migration_folder, migration_list[-1]), header_file)
        header_file.write("\"\n};\n")
        current_schema = open(os.path.join(sql_folder, "schema.sql"), 'r').read()
        current_schema_escaped = escape_string(current_schema)
        header_file.write('extern const std::string %scurrent_schema = "%s";' % (prefix, current_schema_escaped));
        header_file.write('extern const int %scurrent_schema_version = %u;' % (prefix ,(len(migration_list)-1)));
