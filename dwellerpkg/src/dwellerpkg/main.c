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
        "  dwellerpkg build [--static|--shared]    build library from source\n"
        "  dwellerpkg install <file> <name> <ver>  install a library file\n"
        "  dwellerpkg list                         list installed packages\n"
        "  dwellerpkg link <name>                  print linker flags\n"
        "  dwellerpkg metadata <file>              dump metadata from a library\n"
        "  dwellerpkg find <name>                  find a package library\n"
    );
}

static void cmd_init(const char *name) {
    char dir[512];
    snprintf(dir, sizeof(dir), "%s", name);
    mkdir(dir, 0755);

    char src[512];
    snprintf(src, sizeof(src), "%s/main.as", dir);
    FILE *f = fopen(src, "w");
    if (f) {
        fprintf(f, "i32 main(i32* argc, str* argv) {\n");
        fprintf(f, "    return 0;\n");
        fprintf(f, "}\n");
        fclose(f);
    }

    char meta[512];
    snprintf(meta, sizeof(meta), "%s/cpkg.json", dir);
    f = fopen(meta, "w");
    if (f) {
        fprintf(f, "{\n");
        fprintf(f, "  \"name\": \"%s\",\n", name);
        fprintf(f, "  \"version\": \"0.1.0\",\n");
        fprintf(f, "  \"compiler\": \"abyssc\",\n");
        fprintf(f, "  \"source\": [\".\"],\n");
        fprintf(f, "  \"include\": [\".\"]\n");
        fprintf(f, "}\n");
        fclose(f);
    }

    fprintf(stderr, "initialized package '%s' in %s/\n", name, dir);
}

static void cmd_build(int argc, char **argv) {
    int make_static = 1;
    int make_shared = 0;
    const char *output = NULL;
    const char *name = NULL;
    const char *version = NULL;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--static") == 0) make_static = 1;
        else if (strcmp(argv[i], "--shared") == 0) { make_shared = 1; make_static = 0; }
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) output = argv[++i];
        else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) name = argv[++i];
        else if (strcmp(argv[i], "-v") == 0 && i + 1 < argc) version = argv[++i];
    }

    /* Find all .as files in current directory */
    DIR *d = opendir(".");
    if (!d) { fprintf(stderr, "cannot open current directory\n"); return; }

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
        return;
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
    } else if (make_shared) {
        snprintf(out_path, sizeof(out_path), "lib%s.abyss_so", name);
    } else {
        snprintf(out_path, sizeof(out_path), "lib%s.abyss_a", name);
    }

    if (make_static) {
        fprintf(stderr, "building static library %s from %d source files...\n", out_path, source_count);
        if (pkg_build_static(sources, source_count, out_path, name, version))
            fprintf(stderr, "built %s\n", out_path);
        else
            fprintf(stderr, "build failed\n");
    }
    if (make_shared) {
        fprintf(stderr, "building shared library %s from %d source files...\n", out_path, source_count);
        if (pkg_build_shared(sources, source_count, out_path, name, version))
            fprintf(stderr, "built %s\n", out_path);
        else
            fprintf(stderr, "build failed\n");
    }

    for (int i = 0; i < source_count; i++) free((char*)sources[i]);
}

static void cmd_install(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "usage: dwellerpkg install <file> <name> <version>\n");
        return;
    }
    pkg_install(argv[2], argv[3], argv[4]);
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

static void cmd_link(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: dwellerpkg link <name>\n");
        return;
    }
    pkg_print_link_flags(argv[2], stdout);
}

static void cmd_metadata(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: dwellerpkg metadata <file>\n");
        return;
    }

    char *json = NULL;
    size_t json_len = 0;
    void *lib_data = NULL;
    size_t lib_size = 0;

    if (!pkg_read_library(argv[2], &json, &json_len, &lib_data, &lib_size)) {
        fprintf(stderr, "cannot read library '%s'\n", argv[2]);
        return;
    }

    PkgMetadata meta;
    if (pkg_metadata_from_json(json, json_len, &meta)) {
        fprintf(stderr, "package: %s %s\n", meta.name, meta.version);
        fprintf(stderr, "structs: %d\n", meta.struct_count);
        for (int i = 0; i < meta.struct_count; i++) {
            fprintf(stderr, "  struct %s {\n", meta.structs[i].name);
            for (int j = 0; j < meta.structs[i].field_count; j++) {
                fprintf(stderr, "    %s %s%s;\n",
                    meta.structs[i].fields[j].type_name,
                    meta.structs[i].fields[j].is_ptr ? "*" : "",
                    meta.structs[i].fields[j].name);
            }
            fprintf(stderr, "  }\n");
        }
        fprintf(stderr, "functions: %d\n", meta.func_count);
        for (int i = 0; i < meta.func_count; i++) {
            fprintf(stderr, "  %s %s(", meta.funcs[i].return_type, meta.funcs[i].name);
            for (int j = 0; j < meta.funcs[i].param_count; j++) {
                if (j > 0) fprintf(stderr, ", ");
                fprintf(stderr, "%s %s",
                    meta.funcs[i].params[j].type_name,
                    meta.funcs[i].params[j].name);
            }
            fprintf(stderr, ")%s\n", meta.funcs[i].is_extern ? " extern" : "");
        }
        pkg_metadata_free(&meta);
    }

    free(json);
    free(lib_data);
}

static void cmd_find(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: dwellerpkg find <name>\n");
        return;
    }
    char *lib = pkg_find_library(argv[2]);
    if (lib) {
        fprintf(stderr, "%s\n", lib);
        free(lib);
    } else {
        fprintf(stderr, "package '%s' not found\n", argv[2]);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(); return 1; }

    if (strcmp(argv[1], "init") == 0) {
        if (argc < 3) { fprintf(stderr, "usage: dwellerpkg init <name>\n"); return 1; }
        cmd_init(argv[2]);
    } else if (strcmp(argv[1], "build") == 0) {
        cmd_build(argc, argv);
    } else if (strcmp(argv[1], "install") == 0) {
        cmd_install(argc, argv);
    } else if (strcmp(argv[1], "list") == 0) {
        cmd_list();
    } else if (strcmp(argv[1], "link") == 0) {
        cmd_link(argc, argv);
    } else if (strcmp(argv[1], "metadata") == 0) {
        cmd_metadata(argc, argv);
    } else if (strcmp(argv[1], "find") == 0) {
        cmd_find(argc, argv);
    } else {
        fprintf(stderr, "unknown command: %s\n", argv[1]);
        usage();
        return 1;
    }

    return 0;
}
