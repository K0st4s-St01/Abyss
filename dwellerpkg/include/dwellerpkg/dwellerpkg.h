#ifndef DWELLERPKG_H
#define DWELLERPKG_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "../../abyssc/include/ast/ast.h"

/* ── Metadata types ─────────────────────────────────────────── */

typedef struct {
    char *name;
    char *type_name;
    int is_ptr;
} PkgField;

typedef struct {
    char *name;
    char *type_name;
    int is_ptr;
} PkgParam;

typedef struct {
    char *name;
    char *type_name;
    int is_ptr;
} PkgGenericParam;

typedef struct {
    char *name;
    char *return_type;
    PkgParam *params;
    int param_count;
    PkgGenericParam *generic_params;
    int generic_param_count;
    bool is_extern;
} PkgFunc;

typedef struct {
    char *name;
    PkgField *fields;
    int field_count;
    PkgGenericParam *generic_params;
    int generic_param_count;
} PkgStruct;

typedef struct {
    char *name;
    char *version;
    PkgStruct *structs;
    int struct_count;
    PkgFunc *funcs;
    int func_count;
} PkgMetadata;

/* ── Metadata read/write ────────────────────────────────────── */

/* Write metadata + library content to a .abyss_a or .abyss_so file.
   The file format is:
     "DWELLERPKG\0"  (11 bytes magic)
     uint32           metadata JSON length
     <JSON bytes>
     <library content bytes>
 */
bool pkg_write_library(const char *path, const PkgMetadata *meta,
                       const void *lib_data, size_t lib_size);

/* Read metadata from a .abyss_a or .abyss_so file.
   Caller must free *json_out and *lib_out.
   Returns false on failure. */
bool pkg_read_library(const char *path, char **json_out, size_t *json_size,
                      void **lib_out, size_t *lib_size);

/* Parse metadata from a JSON string. */
bool pkg_metadata_from_json(const char *json, size_t len, PkgMetadata *out);

/* Serialize metadata to a JSON string. Caller must free the result. */
char *pkg_metadata_to_json(const PkgMetadata *meta);

/* Free metadata contents. */
void pkg_metadata_free(PkgMetadata *meta);

/* ── Package manager operations ──────────────────────────────── */

/* Get the default package directory (~/.dwellerpkg/packages/). */
const char *pkg_dir(void);

/* Build a .abyss_a static library from source files.
   source_files: array of .as file paths
   source_count: number of source files
   output_path: where to write the .abyss_a file
   package_name: the package/module name
 */
bool pkg_build_static(const char **source_files, int source_count,
                      const char *output_path, const char *package_name,
                      const char *version);

/* Build a .abyss_so shared library from source files. */
bool pkg_build_shared(const char **source_files, int source_count,
                      const char *output_path, const char *package_name,
                      const char *version);

/* Install a library file into the package directory.
   src_path: path to the .abyss_a or .abyss_so file
   package_name: the package name
   version: version string (e.g., "1.0.0")
 */
bool pkg_install(const char *src_path, const char *package_name,
                 const char *version);

/* List installed packages. Returns count. */
int pkg_list_installed(char ***names_out, char ***versions_out);

/* Find a library file for a package name.
   Searches ~/.dwellerpkg/packages/<name>/ for .abyss_a then .abyss_so.
   Caller must free the returned path. */
char *pkg_find_library(const char *package_name);

/* Print linker flags for a package (-L... -l...). */
bool pkg_print_link_flags(const char *package_name, FILE *out);

/* ── Compiler integration ────────────────────────────────────── */

/* Load metadata from a package and inject declarations into the program.
   This is called when the compiler encounters `import <name>;`. */
bool pkg_inject_import(const char *package_name, Program *program);

/* Collect linker flags from all imported packages for the final link step. */
bool pkg_collect_link_flags(const char **imported_packages, int count, FILE *out);

#endif
