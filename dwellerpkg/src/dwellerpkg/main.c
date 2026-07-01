#include "../include/dwellerpkg/dwellerpkg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>

static void usage(void) {
    fprintf(stderr,
        "dwellerpkg - Abyss package manager\n"
        "\n"
        "usage:\n"
        "  dwellerpkg init <name>                  create package skeleton\n"
        "  dwellerpkg build [--static|--shared|--executable]  build library or executable\n"
        "    --link <name>  link against an installed package (repeatable)\n"
        "  dwellerpkg install <file> <name> <ver>  install a library file\n"
        "  dwellerpkg list                         list installed packages\n"
        "  dwellerpkg link <name>                  print linker flags\n"
        "  dwellerpkg metadata <file>              dump metadata from a library\n"
        "  dwellerpkg find <name>                  find a package library\n"
    );
}

static const char *metadata_ptr_suffix(const char *type_name, int is_ptr) {
    if (!is_ptr) return "";
    if (!type_name) return "*";
    size_t len = strlen(type_name);
    return (len > 0 && type_name[len - 1] == '*') ? "" : "*";
}

static int cmd_init(const char *name) {
    char dir[512];
    snprintf(dir, sizeof(dir), "%s", name);
    if (mkdir(dir, 0755) != 0) {
        struct stat st;
        if (stat(dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
            fprintf(stderr, "cannot create package directory '%s'\n", dir);
            return 1;
        }
    }

    char src[512];
    snprintf(src, sizeof(src), "%s/main.as", dir);
    FILE *f = fopen(src, "w");
    if (!f) {
        fprintf(stderr, "cannot create %s\n", src);
        return 1;
    }
    fprintf(f, "i32 main(i32* argc, str* argv) {\n");
    fprintf(f, "    return 0;\n");
    fprintf(f, "}\n");
    fclose(f);

    char meta[512];
    snprintf(meta, sizeof(meta), "%s/cpkg.json", dir);
    f = fopen(meta, "w");
    if (!f) {
        fprintf(stderr, "cannot create %s\n", meta);
        return 1;
    }
    fprintf(f, "{\n");
    fprintf(f, "  \"name\": \"%s\",\n", name);
    fprintf(f, "  \"version\": \"0.1.0\",\n");
    fprintf(f, "  \"compiler\": \"abyssc\",\n");
    fprintf(f, "  \"source\": [\".\"],\n");
    fprintf(f, "  \"include\": [\".\"]\n");
    fprintf(f, "}\n");
    fclose(f);

    fprintf(stderr, "initialized package '%s' in %s/\n", name, dir);
    return 0;
}

static int cmd_build(int argc, char **argv) {
    int make_static = 1;
    int make_shared = 0;
    int make_executable = 0;
    const char *output = NULL;
    const char *name = NULL;
    const char *version = NULL;
    const char *link_libs[64];
    int link_lib_count = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--static") == 0) make_static = 1;
        else if (strcmp(argv[i], "--shared") == 0) { make_shared = 1; make_static = 0; }
        else if (strcmp(argv[i], "--executable") == 0) { make_executable = 1; make_static = 0; }
        else if (strcmp(argv[i], "--link") == 0 && i + 1 < argc) link_libs[link_lib_count++] = argv[++i];
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) output = argv[++i];
        else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) name = argv[++i];
        else if (strcmp(argv[i], "-v") == 0 && i + 1 < argc) version = argv[++i];
    }

    /* Find all .as files in current directory */
    DIR *d = opendir(".");
    if (!d) { fprintf(stderr, "cannot open current directory\n"); return 1; }

    const char *sources[256];
    int source_count = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) && source_count < 256) {
        if (fnmatch("*.as", ent->d_name, 0) == 0) {
            struct stat st;
            if (stat(ent->d_name, &st) == 0 && S_ISREG(st.st_mode)) {
                sources[source_count++] = strdup(ent->d_name);
            }
        }
    }
    closedir(d);

    if (source_count == 0) {
        fprintf(stderr, "no .as files found\n");
        for (int i = 0; i < source_count; i++) free((char*)sources[i]);
        return 1;
    }

    /* Try to read from cpkg.json */
    {
        FILE *f = fopen("cpkg.json", "r");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            rewind(f);
            char *buf = malloc(sz + 1);
            fread(buf, 1, sz, f);
            buf[sz] = '\0';
            fclose(f);

            if (!name) {
                char *p = strstr(buf, "\"name\"");
                if (p) {
                    p = strchr(p, ':');
                    if (p) {
                        p++;
                        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
                        if (*p == '"') {
                            p++;
                            char *end = strchr(p, '"');
                            if (end) {
                                name = strndup(p, end - p);
                            }
                        }
                    }
                }
            }

            if (!version) {
                char *p = strstr(buf, "\"version\"");
                if (p) {
                    p = strchr(p, ':');
                    if (p) {
                        p++;
                        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
                        if (*p == '"') {
                            p++;
                            char *end = strchr(p, '"');
                            if (end) {
                                version = strndup(p, end - p);
                            }
                        }
                    }
                }
            }

            free(buf);
        }
    }
    if (!name) name = "package";
    if (!version) version = "0.0.0";

    char out_path[512];
    if (output) {
        snprintf(out_path, sizeof(out_path), "%s", output);
    } else if (make_executable) {
        snprintf(out_path, sizeof(out_path), "%s", name ? name : "a.out");
    } else if (make_shared) {
        snprintf(out_path, sizeof(out_path), "lib%s.abyss_so", name);
    } else {
        snprintf(out_path, sizeof(out_path), "lib%s.abyss_a", name);
    }

    int status = 1;
    if (make_executable) {
        fprintf(stderr, "building executable %s from %d source files...\n", out_path, source_count);
        if (pkg_build_executable(sources, source_count, out_path, link_libs, link_lib_count)) {
            fprintf(stderr, "built %s\n", out_path);
            status = 0;
        } else {
            fprintf(stderr, "build failed\n");
        }
    } else if (make_static) {
        fprintf(stderr, "building static library %s from %d source files...\n", out_path, source_count);
        if (pkg_build_static(sources, source_count, out_path, name, version)) {
            fprintf(stderr, "built %s\n", out_path);
            status = 0;
        } else {
            fprintf(stderr, "build failed\n");
        }
    } else if (make_shared) {
        fprintf(stderr, "building shared library %s from %d source files...\n", out_path, source_count);
        if (pkg_build_shared(sources, source_count, out_path, name, version)) {
            fprintf(stderr, "built %s\n", out_path);
            status = 0;
        } else {
            fprintf(stderr, "build failed\n");
        }
    }

    for (int i = 0; i < source_count; i++) free((char*)sources[i]);
    return status;
}

static int cmd_install(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "usage: dwellerpkg install <file> <name> <version>\n");
        return 1;
    }
    return pkg_install(argv[2], argv[3], argv[4]) ? 0 : 1;
}

static void cmd_list(void) {
    char **names, **versions;
    int count = pkg_list_installed(&names, &versions);
    if (count == 0) {
        fprintf(stderr, "no packages installed\n");
    } else {
        for (int i = 0; i < count; i++) {
            fprintf(stderr, "  %s %s\n", names[i], versions[i]);
            free(names[i]);
            free(versions[i]);
        }
    }
    free(names);
    free(versions);
}

static int cmd_link(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: dwellerpkg link <name>\n");
        return 1;
    }
    return pkg_print_link_flags(argv[2], stdout) ? 0 : 1;
}

static int cmd_metadata(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: dwellerpkg metadata <file>\n");
        return 1;
    }

    char *json = NULL;
    size_t json_len = 0;
    void *lib_data = NULL;
    size_t lib_size = 0;

    if (!pkg_read_library(argv[2], &json, &json_len, &lib_data, &lib_size)) {
        fprintf(stderr, "cannot read library '%s'\n", argv[2]);
        return 1;
    }

    int status = 1;
    PkgMetadata meta;
    if (pkg_metadata_from_json(json, json_len, &meta)) {
        fprintf(stderr, "package: %s %s\n", meta.name, meta.version);
        fprintf(stderr, "type aliases: %d\n", meta.alias_count);
        for (int i = 0; i < meta.alias_count; i++) {
            fprintf(stderr, "  type %s = %s\n",
                    meta.aliases[i].name,
                    meta.aliases[i].target_type);
        }
        fprintf(stderr, "enums: %d\n", meta.enum_count);
        for (int i = 0; i < meta.enum_count; i++) {
            fprintf(stderr, "  enum %s {\n", meta.enums[i].name);
            for (int j = 0; j < meta.enums[i].variant_count; j++) {
                fprintf(stderr, "    %s = %d\n",
                        meta.enums[i].variants[j].name,
                        meta.enums[i].variants[j].value);
            }
            fprintf(stderr, "  }\n");
        }
        fprintf(stderr, "interfaces: %d\n", meta.interface_count);
        for (int i = 0; i < meta.interface_count; i++) {
            fprintf(stderr, "  interface %s {\n", meta.interfaces[i].name);
            for (int j = 0; j < meta.interfaces[i].method_count; j++) {
                fprintf(stderr, "    %s %s(",
                        meta.interfaces[i].methods[j].return_type,
                        meta.interfaces[i].methods[j].name);
                for (int k = 0; k < meta.interfaces[i].methods[j].param_count; k++) {
                    if (k > 0) fprintf(stderr, ", ");
                    fprintf(stderr, "%s %s%s",
                            meta.interfaces[i].methods[j].params[k].type_name,
                            metadata_ptr_suffix(
                                meta.interfaces[i].methods[j].params[k].type_name,
                                meta.interfaces[i].methods[j].params[k].is_ptr),
                            meta.interfaces[i].methods[j].params[k].name);
                }
                if (meta.interfaces[i].methods[j].is_variadic) {
                    if (meta.interfaces[i].methods[j].param_count > 0)
                        fprintf(stderr, ", ");
                    fprintf(stderr, "...");
                }
                fprintf(stderr, ")\n");
            }
            fprintf(stderr, "  }\n");
        }
        fprintf(stderr, "structs: %d\n", meta.struct_count);
        for (int i = 0; i < meta.struct_count; i++) {
            fprintf(stderr, "  struct %s {\n", meta.structs[i].name);
            for (int j = 0; j < meta.structs[i].field_count; j++) {
                fprintf(stderr, "    %s %s%s",
                    meta.structs[i].fields[j].type_name,
                    metadata_ptr_suffix(meta.structs[i].fields[j].type_name,
                                        meta.structs[i].fields[j].is_ptr),
                    meta.structs[i].fields[j].name);
                if (meta.structs[i].fields[j].array_size > 0)
                    fprintf(stderr, "[%d]", meta.structs[i].fields[j].array_size);
                fprintf(stderr, ";\n");
            }
            for (int j = 0; j < meta.structs[i].method_count; j++) {
                fprintf(stderr, "    %s%s %s(",
                        meta.structs[i].methods[j].is_static ? "static " : "",
                        meta.structs[i].methods[j].return_type,
                        meta.structs[i].methods[j].name);
                for (int k = 0; k < meta.structs[i].methods[j].param_count; k++) {
                    if (k > 0) fprintf(stderr, ", ");
                    fprintf(stderr, "%s %s%s",
                            meta.structs[i].methods[j].params[k].type_name,
                            metadata_ptr_suffix(
                                meta.structs[i].methods[j].params[k].type_name,
                                meta.structs[i].methods[j].params[k].is_ptr),
                            meta.structs[i].methods[j].params[k].name);
                }
                fprintf(stderr, ")%s%s\n",
                        meta.structs[i].methods[j].is_variadic ? " variadic" : "",
                        meta.structs[i].methods[j].is_extern ? " extern" : "");
            }
            fprintf(stderr, "  }\n");
        }
        fprintf(stderr, "globals: %d\n", meta.global_count);
        for (int i = 0; i < meta.global_count; i++) {
            fprintf(stderr, "  %s %s%s",
                    meta.globals[i].type_name,
                    metadata_ptr_suffix(meta.globals[i].type_name,
                                        meta.globals[i].is_ptr),
                    meta.globals[i].name);
            if (meta.globals[i].array_size > 0)
                fprintf(stderr, "[%d]", meta.globals[i].array_size);
            fprintf(stderr, "%s\n", meta.globals[i].is_extern ? " extern" : "");
        }
        fprintf(stderr, "functions: %d\n", meta.func_count);
        for (int i = 0; i < meta.func_count; i++) {
            fprintf(stderr, "  %s%s %s(",
                    meta.funcs[i].is_static ? "static " : "",
                    meta.funcs[i].return_type,
                    meta.funcs[i].name);
            for (int j = 0; j < meta.funcs[i].param_count; j++) {
                if (j > 0) fprintf(stderr, ", ");
                fprintf(stderr, "%s %s%s",
                    meta.funcs[i].params[j].type_name,
                    metadata_ptr_suffix(meta.funcs[i].params[j].type_name,
                                        meta.funcs[i].params[j].is_ptr),
                    meta.funcs[i].params[j].name);
            }
            fprintf(stderr, ")%s%s\n",
                    meta.funcs[i].is_variadic ? " variadic" : "",
                    meta.funcs[i].is_extern ? " extern" : "");
        }
        pkg_metadata_free(&meta);
        status = 0;
    } else {
        fprintf(stderr, "cannot parse metadata from '%s'\n", argv[2]);
    }

    free(json);
    free(lib_data);
    return status;
}

static int cmd_find(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: dwellerpkg find <name>\n");
        return 1;
    }
    char *lib = pkg_find_library(argv[2]);
    if (lib) {
        fprintf(stderr, "%s\n", lib);
        free(lib);
        return 0;
    } else {
        fprintf(stderr, "package '%s' not found\n", argv[2]);
        return 1;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(); return 1; }

    if (strcmp(argv[1], "init") == 0) {
        if (argc < 3) { fprintf(stderr, "usage: dwellerpkg init <name>\n"); return 1; }
        return cmd_init(argv[2]);
    } else if (strcmp(argv[1], "build") == 0) {
        return cmd_build(argc, argv);
    } else if (strcmp(argv[1], "install") == 0) {
        return cmd_install(argc, argv);
    } else if (strcmp(argv[1], "list") == 0) {
        cmd_list();
    } else if (strcmp(argv[1], "link") == 0) {
        return cmd_link(argc, argv);
    } else if (strcmp(argv[1], "metadata") == 0) {
        return cmd_metadata(argc, argv);
    } else if (strcmp(argv[1], "find") == 0) {
        return cmd_find(argc, argv);
    } else {
        fprintf(stderr, "unknown command: %s\n", argv[1]);
        usage();
        return 1;
    }

    return 0;
}
