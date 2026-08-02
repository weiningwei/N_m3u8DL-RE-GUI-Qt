#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// Describes a single N_m3u8DL-RE command-line option so the UI can be
// generated declaratively from the documentation in doc/README.md.
struct Opt {
    enum Type {
        Bool,       // a switch: emitted only when checked
        String,     // free text: --flag <value>
        PathFile,   // file path (with file picker)
        PathDir,    // directory path (with folder picker)
        Int,        // integer spin box
        Combo,      // drop down with a fixed set of choices
        List        // repeatable value, e.g. -H / --key / --mux-import
    };

    Type type = String;
    QString flag;         // command-line flag, e.g. "--save-dir" or "-H"
    QString label;        // human readable label (Chinese)
    QString tooltip;      // help text taken from the documentation
    QString def;          // documented default (informational only)
    QStringList choices;  // for Combo
    int intDefault = 0;   // default for Int (omit flag when equal)
    bool boolDefault = false; // default checked state for Bool
};

struct Category {
    QString name;            // tab title
    QVector<Opt> opts;
};

// Builds the full option tree from the documented CLI parameters.
QVector<Category> buildCategories();
