// A column in a component table ("CTable"), containing components
// of a specific type.
// Columns have very limited and restricted data and functionality,
// as they are intended to be mostly handled by their owner tables.

#ifndef COLUMN_H
#define COLUMN_H

#include "../tools/mem_tools.h"
#include "../interface/ct.h"

#define COL_FS "col%s"
#define COL_FA(col) ""

// The table owning the column is responsible for keeping track
// of the column's capacity and component type.
struct Column {
        // A buffer of components for the column.
        void * components;
        // The size of a single component.
        // Could be evaluated by the table owning the column
        // since that table keeps track of the type of the
        // components in the column, but this is a lot simpler
        // and a little bit faster, at the cost of (usually)
        // 8 bytes (Linux uses 4-byte "size_t"s, but with
        // a pointer member and padding the size should still
        // be 8. Also, I don't use Linux and never will).
        size_t component_size;
};

// Creates a column with capacity for "capacity" individual components
// of type "ct". No components are initialized.
struct Column create_column(struct Ct ct, size_t capacity);

// Resizes a column to "count" components.
// No components will be destroyed, even if "col" is resized to a
// size lower than its number of components (in fact, "col" doesn't
// even know how many components it actually contains).
void resize_column(struct Column * col, size_t count);

#endif
