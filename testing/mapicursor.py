# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 1997 - July 2008 CWI, August 2008 - 2016 MonetDB B.V.

import logging
from collections import namedtuple
from typing import Optional, Dict
from pymonetdb.sql.debug import debug, export
from pymonetdb.sql import monetize
from pymonetdb.sql.pythonize import strip, py_bool, py_date, py_time, py_timestamp
from pymonetdb.exceptions import ProgrammingError, InterfaceError
from pymonetdb import mapi
import uuid
import json

logger = logging.getLogger("pymonetdb")

Description = namedtuple('Description', ('name', 'type_code', 'display_size', 'internal_size', 'precision', 'scale',
                                         'null_ok'))
def no_convert(data):
    return data

def py_oid(data):
    return int(data[:-2])

mapping = {
    'none': no_convert,
    'void': strip,
    'str': strip,
    'int': int,
    'oid': py_oid,
    'flt': float,
    'dbl': float,
    'bit': py_bool,
    'date': py_date,
    'daytime': py_time,
    'timestamp': py_timestamp,
    'url': strip,
    'uuid': uuid.UUID,
    'json': json.loads,
    'color': strip,
}

def convert(data, type_code):
    """
    Calls the appropriate convertion function based upon the python type
    """

    # null values should always be replaced by None type
    if data == "nil":
        return None
    try:
        return mapping[type_code](data)
    except KeyError:
        raise ProgrammingError("type %s is not supported" % type_code)

class MapiCursor(object):
    """This object represents a database cursor, which is used to manage
    the context of a fetch operation. Cursors created from the same
    connection are not isolated, i.e., any changes done to the
    database by a cursor are immediately visible by the other
    cursors"""

    def __init__(self, connection):
        """This read-only attribute return a reference to the Connection
        object on which the cursor was created."""
        self.connection = connection

        # last executed operation (query)
        self.operation = ""

        # This read-only attribute specifies the number of rows that
        # the last .execute*() produced (for DQL statements like
        # 'select') or affected (for DML statements like 'update' or
        # 'insert').
        #
        # The attribute is -1 in case no .execute*() has been
        # performed on the cursor or the rowcount of the last
        # operation is cannot be determined by the interface.
        self.rowcount = -1

        # This read-only attribute is a sequence of 7-item
        # sequences.
        #
        # Each of these sequences contains information describing
        # one result column:
        #
        #   (name,
        #    type_code,
        #    display_size,
        #    internal_size,
        #    precision,
        #    scale,
        #    null_ok)
        #
        # This attribute will be None for operations that
        # do not return rows or if the cursor has not had an
        # operation invoked via the .execute*() method yet.
        self.description = None

        # This read-only attribute indicates at which row
        # we currently are
        self.rownumber = -1

        self._executed = None

        # the offset of the current resultset in the total resultset
        self._offset = 0

        # the resultset
        self._rows = []

        # used to identify a query during server contact.
        # Only select queries have query ID
        self._query_id = -1

        # This is a Python list object to which the interface appends
        # tuples (exception class, exception value) for all messages
        # which the interfaces receives from the underlying database for
        # this cursor.
        #
        # The list is cleared by all standard cursor methods calls (prior
        # to executing the call) except for the .fetch*() calls
        # automatically to avoid excessive memory usage and can also be
        # cleared by executing "del cursor.messages[:]".
        #
        # All error and warning messages generated by the database are
        # placed into this list, so checking the list allows the user to
        # verify correct operation of the method calls.
        self.messages = []

        # This read-only attribute provides the rowid of the last
        # modified row (most databases return a rowid only when a single
        # INSERT operation is performed). If the operation does not set
        # a rowid or if the database does not support rowids, this
        # attribute should be set to None.
        #
        # The semantics of .lastrowid are undefined in case the last
        # executed statement modified more than one row, e.g. when
        # using INSERT with .executemany().
        self.lastrowid = None

    def _check_executed(self):
        if not self._executed:
            self._exception_handler(ProgrammingError, "do a execute() first")

    def close(self):
        """ Close the cursor now (rather than whenever __del__ is
        called).  The cursor will be unusable from this point
        forward; an Error (or subclass) exception will be raised
        if any operation is attempted with the cursor."""
        self.connection = None

    def execute(self, operation, parameters=None):
        # type: (str, Optional[Dict]) -> int
        """Prepare and execute a database operation (query or
        command).  Parameters may be provided as mapping and
        will be bound to variables in the operation.
        """

        self._query_id = -1
        self.description = None
        self._rows = []

        if not self.connection:
            self._exception_handler(ProgrammingError, "cursor is closed")

        # clear message history
        self.messages = []

        if operation == self.operation:
            # same operation, DBAPI mentioned something about reuse
            # but monetdb doesn't support this
            pass
        else:
            self.operation = operation

        query = ""
        if parameters:
            if isinstance(parameters, dict):
                query = operation % {k: monetize.convert(v) for (k, v) in parameters.items()}
            elif type(parameters) == list or type(parameters) == tuple:
                query = operation % tuple(
                    [monetize.convert(item) for item in parameters])
            elif isinstance(parameters, str):
                query = operation % monetize.convert(parameters)
            else:
                msg = "Parameters should be None, dict or list, now it is %s"
                self._exception_handler(ValueError, msg % type(parameters))
        else:
            query = operation

        if query[0] == '#': # skip comment
            return 0
        if query[-2:] != ';\n':
            query += ';\n'
        if query[-1:] != '\n':
            query += '\n'
        block = self.connection.cmd(query)
        self._store_result(block)
        self.rownumber = 0
        self._executed = operation
        return self.rowcount

    def executemany(self, operation, seq_of_parameters):
        """Prepare a database operation (query or command) and then
        execute it against all parameter sequences or mappings
        found in the sequence seq_of_parameters.

        It will return the number or rows affected
        """

        count = 0
        for parameters in seq_of_parameters:
            count += self.execute(operation, parameters)
        self.rowcount = count
        return count

    def debug(self, query, fname, sample=-1):
        """ Locally debug a given Python UDF function in a SQL query
            using the PDB debugger. Optionally can run on only a
            sample of the input data, for faster data export.
        """
        debug(self, query, fname, sample)

    def export(self, query, fname, sample=-1, filespath='./'):
        return export(self, query, fname, sample, filespath)

    def fetchone(self):
        """Fetch the next row of a query result set, returning a
        single sequence, or None when no more data is available."""

        self._check_executed()

        if self._query_id == -1:
            msg = "query didn't result in a resultset"
            self._exception_handler(ProgrammingError, msg)

        if self.rownumber >= self.rowcount:
            return None

        if self.rownumber >= (self._offset + len(self._rows)):
            self.nextset()

        result = self._rows[self.rownumber - self._offset]
        self.rownumber += 1
        return result

    def fetchmany(self, size=None):
        """Fetch the next set of rows of a query result, returning a
        sequence of sequences (e.g. a list of tuples). An empty
        sequence is returned when no more rows are available.

        The number of rows to fetch per call is specified by the
        parameter.  If it is not given, the cursor's arraysize
        determines the number of rows to be fetched. The method
        should try to fetch as many rows as indicated by the size
        parameter. If this is not possible due to the specified
        number of rows not being available, fewer rows may be
        returned.

        An Error (or subclass) exception is raised if the previous
        call to .execute*() did not produce any result set or no
        call was issued yet.

        Note there are performance considerations involved with
        the size parameter.  For optimal performance, it is
        usually best to use the arraysize attribute.  If the size
        parameter is used, then it is best for it to retain the
        same value from one .fetchmany() call to the next."""

        self._check_executed()

        if self.rownumber >= self.rowcount:
            return []

        end = min(self.rownumber + (size or self.arraysize), self.rowcount)
        result = self._rows[self.rownumber - self._offset:end - self._offset]
        self.rownumber = min(end, len(self._rows) + self._offset)

        while (end > self.rownumber) and self.nextset():
            result += self._rows[self.rownumber - self._offset:end - self._offset]
            self.rownumber = min(end, len(self._rows) + self._offset)
        return result

    def fetchall(self):
        """Fetch all (remaining) rows of a query result, returning
        them as a sequence of sequences (e.g. a list of tuples).
        Note that the cursor's arraysize attribute can affect the
        performance of this operation.

        An Error (or subclass) exception is raised if the previous
        call to .execute*() did not produce any result set or no
        call was issued yet."""

        self._check_executed()

        if self._query_id == -1:
            msg = "query didn't result in a resultset"
            self._exception_handler(ProgrammingError, msg)

        result = self._rows[self.rownumber - self._offset:]
        self.rownumber = len(self._rows) + self._offset

        # slide the window over the resultset
        while self.nextset():
            result += self._rows
            self.rownumber = len(self._rows) + self._offset

        return result

    def nextset(self):
        """This method will make the cursor skip to the next
        available set, discarding any remaining rows from the
        current set.

        If there are no more sets, the method returns
        None. Otherwise, it returns a true value and subsequent
        calls to the fetch methods will return rows from the next
        result set.

        An Error (or subclass) exception is raised if the previous
        call to .execute*() did not produce any result set or no
        call was issued yet."""

        self._check_executed()

        if self.rownumber >= self.rowcount:
            return False

        self._offset += len(self._rows)

        end = min(self.rowcount, self.rownumber + self.arraysize)
        amount = end - self._offset

        command = 'Xexport %s %s %s' % (self._query_id, self._offset, amount)
        block = self.connection.command(command)
        self._store_result(block)
        return True

    def setinputsizes(self, sizes):
        """
        This method would be used before the .execute*() method
        is invoked to reserve memory. This implementation doesn't
        use this.
        """
        pass

    def setoutputsize(self, size, column=None):
        """
        Set a column buffer size for fetches of large columns
        This implementation doesn't use this
        """
        pass

    def __iter__(self):
        return self

    def next(self):
        row = self.fetchone()
        if not row:
            raise StopIteration
        return row

    def __next__(self):
        return self.next()

    def _store_result(self, block):
        """ parses the mapi result into a resultset"""

        if not block:
            block = ""

        columns = 0
        column_name = ""
        scale = display_size = internal_size = precision = 0
        null_ok = False
        type_ = []

        for line in block.split("\n"):
            if line.startswith(mapi.MSG_QTABLE):
                self._query_id, rowcount, columns, tuples = line[2:].split()[:4]

                columns = int(columns)  # number of columns in result
                self.rowcount = int(rowcount)  # total number of rows
                # tuples = int(tuples)     # number of rows in this set
                self._rows = []

                # set up fields for description
                # table_name = [None] * columns
                column_name = [None] * columns
                type_ = [None] * columns
                display_size = [None] * columns
                internal_size = [None] * columns
                precision = [None] * columns
                scale = [None] * columns
                null_ok = [None] * columns
                # typesizes = [(0, 0)] * columns

                self._offset = 0
                self.lastrowid = None

            elif line.startswith(mapi.MSG_INFO) and (line[0:2] == "#-" or line[0:2] == "# "):
                if line[0:2] == "#-":
                    continue
                (data, identity) = line[1:].split("#")
                values = [x.strip() for x in data.split("\t")]
                identity = identity.strip()

                if identity == "name":
                    column_name = values
                elif identity == "table_name":
                    _ = values  # not used
                elif identity == "type":
                    type_ = values
                elif identity == "length":
                    _ = values  # not used
                elif identity == "typesizes":
                    typesizes = [[int(j) for j in i.split()] for i in values]
                    internal_size = [x[0] for x in typesizes]
                    for num, typeelem in enumerate(type_):
                        if typeelem in ['decimal']:
                            precision[num] = typesizes[num][0]
                            scale[num] = typesizes[num][1]
                else:
                    msg = "unknown header field: {}".format(identity)
                    logger.warning(msg)
                    self.messages.append((Warning, msg))

                if identity != "type":
                    continue

                description = []
                for i in range(len(column_name)):
                    description.append(Description(column_name[i], type_[i], 10, 10, 10, 3, 0))
                self.description = description
                self._offset = 0
                self._query_id = 0
                self.lastrowid = None
                self.rowcount = 0

            elif line.startswith(mapi.MSG_TUPLE):
                values = self._parse_tuple(line)
                self._rows.append(values)
                self.rowcount = len(self._rows)

            elif line.startswith(mapi.MSG_TUPLE_NOSLICE):
                self._rows.append((line[1:],))

            elif line.startswith(mapi.MSG_QBLOCK):
                self._rows = []

            elif line.startswith(mapi.MSG_QSCHEMA):
                self._offset = 0
                self.lastrowid = None
                self._rows = []
                self.description = None
                self.rowcount = -1

            elif line.startswith(mapi.MSG_QUPDATE):
                (affected, identity) = line[2:].split()[:2]
                self._offset = 0
                self._rows = []
                self.description = None
                self.rowcount = int(affected)
                self.lastrowid = int(identity)
                self._query_id = -1

            elif line.startswith(mapi.MSG_QTRANS):
                self._offset = 0
                self.lastrowid = None
                self._rows = []
                self.description = None
                self.rowcount = -1

            elif line == mapi.MSG_PROMPT:
                return

            elif line.startswith(mapi.MSG_ERROR):
                self._exception_handler(ProgrammingError, line[1:])

            else: # single line result (not protocol issue)
                self._offset = 0
                self.lastrowid = None
                self._rows = [ tuple([ line ] ) ]
                self.description = [ Description('value', 'none', 10, 10, 10, 3, 0) ]
                self.rowcount = 1
                self._query_id = 1

        self._exception_handler(InterfaceError, "Unknown state, %s" % block)

    def _parse_tuple(self, line):
        """
        parses a mapi data tuple, and returns a list of python types
        """
        elements = line[1:-1].split(',\t')
        # fallback for output without headers
        if not self.description:
            self.description = []
            self.description.append(Description('b', 'none', 10, 10, 0, 0, 0))
        if len(elements) == len(self.description):
            self._query_id = 2
            return tuple([convert(element.strip(), description[1])
                          for (element, description) in zip(elements,
                                                            self.description)])
        else:
            self._exception_handler(InterfaceError, "length of row doesn't match header")

    def scroll(self, value, mode='relative'):
        """
        Scroll the cursor in the result set to a new position according
        to mode.

        If mode is 'relative' (default), value is taken as offset to
        the current position in the result set, if set to 'absolute',
        value states an absolute target position.

        An IndexError is raised in case a scroll operation would
        leave the result set.
        """
        self._check_executed()

        if mode not in ['relative', 'absolute']:
            msg = "unknown mode '%s'" % mode
            self._exception_handler(ProgrammingError, msg)

        if mode == 'relative':
            value += self.rownumber

        if value > self.rowcount:
            self._exception_handler(IndexError, "value beyond length of resultset")

        self._offset = value
        end = min(self.rowcount, self.rownumber + self.arraysize)
        amount = end - self._offset
        command = 'Xexport %s %s %s' % (self._query_id, self._offset, amount)
        block = self.connection.command(command)
        self._store_result(block)

    def _exception_handler(self, exception_class, message):
        """
        raises the exception specified by exception, and add the error
        to the message list
        """
        self.messages.append((exception_class, message))
        raise exception_class(message)
