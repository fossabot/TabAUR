#include <memory>
#pragma GCC diagnostic ignored "-Wvla"

#include "util.hpp"
#include "args.hpp"
#include "taur.hpp"

#define BRANCH "libalpm-test"
#define VERSION "0.6.0"

std::unique_ptr<Config> config;
std::unique_ptr<TaurBackend> backend;

// this may be hard to read, but better than calling fmt::println multiple times
void usage(int op) {
    if(op == OP_MAIN) {
        fmt::println("TabAUR usage: taur <op> [...]");
        fmt::print("options:\n{}", R"#(
    taur {-h --help}
    taur {-V --version}
    taur {-t, --test-colors}
    taur {-D --database} <options> <package(s)>
    taur {-F --files}    [options] [file(s)]
    taur {-Q --query}    [options] [package(s)]
    taur {-R --remove}   [options] <package(s)>
    taur {-S --sync}     [options] [package(s)]
    taur {-T --deptest}  [options] [package(s)]
    taur {-U --upgrade}  [options] <file(s)>
    )#");
    }
    else {
        if(op == OP_SYNC) {
            fmt::println("usage:  taur {{-S --sync}} [options] [package(s)]");
            fmt::println("options:{}", R"#(
    -s, --search <regex> search remote repositories for matching strings
    -u, --sysupgrade     upgrade installed packages (-uu enables downgrades)
    -y, --refresh        download fresh package databases from the server
            )#");
        }
        else if (op == OP_QUERY) {
            fmt::println("usage: taur {{-Q --query}} [options] [package(s)]");
            fmt::print("options:{}", R"#(
    -q, --quiet          show less information for query and search
                         )#");
        }
    }

    fmt::println("{}", R"#(
    -a, --aur-only       do only AUR operations
    -g, --use-git        use git instead of tarballs for AUR repos
    --config    <path>   set an alternate configuration file
    --theme     <path>   set an alternate theme file
    --cachedir  <dir>    set an alternate package cache location
    --colors    <1,0>    colorize the output
    --debug     <1,0>    show debug messages
    --sudo      <path>   choose which binary to use for privilage-escalation
    )#");
    fmt::println("TabAUR will assume -Syu if you pass no arguments to it.");

    if (config->secretRecipe) {
        log_printf(LOG_INFO, "Loading secret recipe...\n");
        for (auto const& i : secret) {
            fmt::println("{}", i);
            usleep(650000); // 0.65 seconds
        }
    }
}

void test_colors() {
    TaurPkg_t pkg = { 
                        "TabAUR", // name
                        VERSION,  // version
                        "https://github.com/BurntRanch/TabAUR", // url
                        "A customizable and lightweight AUR helper, designed to be simple but powerful.", // desc
                        10,  // Popularity
                        vector<string>(),   // depends
                        "aur", // db_name
                    };

    if(fmt::disable_colors)
        fmt::println("Colors are disabled");
    log_printf(LOG_DEBUG, "Debug color: {}\n",  fmt::format(BOLD_TEXT(config->getThemeValue("magenta", magenta)), "(bold) magenta"));
    log_printf(LOG_INFO, "Info color: {}\n",    fmt::format(BOLD_TEXT(config->getThemeValue("cyan", cyan)), "(bold) cyan"));
    log_printf(LOG_WARN, "Warning color: {}\n", fmt::format(BOLD_TEXT(config->getThemeValue("yellow", yellow)), "(bold) yellow"));
    log_printf(LOG_ERROR, "Error color: {}\n",  fmt::format(BOLD_TEXT(config->getThemeValue("red", red)), "(bold) red"));
    fmt::println(fg(config->getThemeValue("red", red)), "red");
    fmt::println(fg(config->getThemeValue("blue", blue)), "blue");
    fmt::println(fg(config->getThemeValue("yellow", yellow)), "yellow");
    fmt::println(fg(config->getThemeValue("green", green)), "green");
    fmt::println(fg(config->getThemeValue("cyan", cyan)), "cyan");
    fmt::println(fg(config->getThemeValue("magenta", magenta)), "magenta");

    fmt::println("\ndb colors:");
    fmt::println(getColorFromDBName("aur"), "(bold) aur");
    fmt::println(getColorFromDBName("extra"), "(bold) extra");
    fmt::println(getColorFromDBName("core"), "(bold) core");
    fmt::println(getColorFromDBName("multilib"), "(bold) multilib");
    fmt::println(getColorFromDBName("others"), "(bold) others");

    fmt::println(BOLD_TEXT(config->getThemeValue("version", green)), "\n(bold) version " VERSION);
    fmt::println(fg(config->getThemeValue("popularity", cyan)), "Popularity: {} ({})", pkg.popularity, getTitleForPopularity(pkg.popularity));
    fmt::println(fg(config->getThemeValue("index", magenta)), "index [1]");

    fmt::println("\nexamples package search preview:");
    printPkgInfo(pkg, pkg.db_name);
}

bool execPacman(int argc, char* argv[]) {
    log_printf(LOG_DEBUG, "Passing command to pacman! (argc: {:d})\n", argc);
    
    if (0 > argc || argc > 512)
        throw std::invalid_argument("argc is invalid! (512 > argc > 0)");

    char* args[argc+3]; // sudo + pacman + null terminator

    args[0] = _(config->sudo.c_str());
    args[1] = _("pacman"); // The command to run as superuser (pacman)
    for (int i = 0; i < argc; ++i) {
        log_printf(LOG_DEBUG, "args[{}] = argv[{}] ({})\n", i + 2, i, argv[i]);
        args[i+2] = argv[i];
    }

    args[argc+2] = nullptr; // null-terminate the array

    execvp(args[0], args);

    // If execvp returns, it means an error occurred
    perror("execvp");
    return false;
}

int installPkg(string pkgName) { 
    bool            useGit   = config->useGit;
    string          cacheDir = config->cacheDir;

    vector<TaurPkg_t>  pkgs = backend->search(pkgName, useGit);

    if (pkgs.empty() && !op.op_s_upgrade) {
        log_printf(LOG_WARN, "No results found, Exiting!\n");
        return false;
    } else if (pkgs.empty()) {
        if (!config->aurOnly) {
            vector<const char *> cmd = {config->sudo.c_str(), "pacman", "-S"};
            if(op.op_s_sync)
                cmd.push_back("-y");
            if(op.op_s_upgrade)
                cmd.push_back("-u");
            
            if (!taur_exec(cmd))
                return false;
        }
        return backend->update_all_pkgs(cacheDir, useGit);
    }

    // ./taur -Ss -- list only, don't install.
    if (op.op_s_search) {
        for (size_t i = 0; i < pkgs.size(); i++)
            printPkgInfo(pkgs[i], pkgs[i].db_name);
        return true;
    }

    optional<vector<TaurPkg_t>> oSelectedPkgs = askUserForPkg(pkgs, *backend, useGit);

    if (!oSelectedPkgs)
        return false;

    vector<TaurPkg_t> selectedPkgs = oSelectedPkgs.value();

    for (size_t i = 0; i < selectedPkgs.size(); i++) {
        TaurPkg_t pkg = selectedPkgs[i];

        string url = pkg.url;

        if (url == "") {
            vector<const char *> cmd = {config->sudo.c_str(), "pacman", "-S"};
            if(op.op_s_sync)
                cmd.push_back("-y");
            if(op.op_s_upgrade)
                cmd.push_back("-u");

            cmd.push_back(pkg.name.c_str());

            cmd.push_back(NULL);

            if (taur_exec(cmd))
                continue;
            else
                return false;
        }

        string filename = path(cacheDir) / url.substr(url.rfind("/") + 1);

        if (useGit)
            filename = filename.substr(0, filename.rfind(".git"));

        bool stat = backend->download_pkg(url, filename);

        if (!stat) {
            log_printf(LOG_ERROR, "An error has occurred and we could not download your package.\n");
            return false;
        }

        if (!useGit){
            stat = backend->handle_aur_depends(pkg, cacheDir, backend->get_all_local_pkgs(true), useGit);
            stat = backend->install_pkg(pkg.name, filename.substr(0, filename.rfind(".tar.gz")), false);
        }
        else {
            stat = backend->handle_aur_depends(pkg, cacheDir, backend->get_all_local_pkgs(true), useGit);
            stat = backend->install_pkg(pkg.name, filename, false);
        }

        if (!stat) {
            log_printf(LOG_ERROR, "Building your package has failed.\n");
            return false;
        }

        log_printf(LOG_DEBUG, "Installing {}.\n", pkg.name);
        if (!taur_exec({config->sudo.c_str(), "pacman", "-U", built_pkg.c_str()})) {
            log_printf(LOG_ERROR, "Failed to install {}.\n", pkg.name);
            return false;
        }

    }

    if (op.op_s_upgrade) {
        log_printf(LOG_INFO, "-u flag specified, upgrading AUR packages.\n");
        return backend->update_all_pkgs(cacheDir, useGit);
    }

    return true;
}

bool removePkg(string pkgName) {
    return backend->remove_pkg(pkgName, config->aurOnly);
}

bool updateAll() {
    return backend->update_all_pkgs(config->cacheDir, config->useGit);
}

bool queryPkgs() {
    log_printf(LOG_DEBUG, "AUR Only: {}\n", config->aurOnly);
    alpm_list_t *pkg;

    vector<const char *> pkgs, pkgs_ver;
    for (pkg = alpm_db_get_pkgcache(alpm_get_localdb(config->handle)); pkg; pkg = alpm_list_next(pkg)){
        pkgs.push_back(alpm_pkg_get_name((alpm_pkg_t *)(pkg->data)));
        pkgs_ver.push_back(alpm_pkg_get_version((alpm_pkg_t *)(pkg->data)));
    }

    if (config->aurOnly) {
        alpm_list_t *syncdbs = alpm_get_syncdbs(config->handle);

        if (!syncdbs) {
            log_printf(LOG_ERROR, "Failed to get syncdbs!\n");
            return false;
        }

        for (; syncdbs; syncdbs = syncdbs->next)
            for (size_t i = 0; i < pkgs.size(); i++)
                if (alpm_db_get_pkg((alpm_db_t *)(syncdbs->data), pkgs[i]))
                    pkgs[i] = NULL; // wont be printed
    }

    if(config->quiet) {
        for (size_t i = 0; i < pkgs.size(); i++) {
            if (!pkgs[i])
                continue;
            fmt::println("{}", pkgs[i]);
        }
    } else {
        fmt::rgb _green = config->getThemeValue("version", green);
        for(size_t i = 0; i < pkgs.size(); i++) {
            if (!pkgs[i])
                continue;
            fmt::print(fmt::emphasis::bold, "{} ", pkgs[i]);
            fmt::println(BOLD_TEXT(_green), "{}", pkgs_ver[i]);
        }
    }

    return true;
}

int parseargs(int argc, char* argv[]) {
    // default
    op.op = OP_MAIN;

    int opt = 0;
    int option_index = 0;
    int result = 0;
    const char *optstring = "DFQRSTUVahqsuyt";
    static const struct option opts[] =
    {
        {"database",   no_argument,       0, 'D'},
        {"files",      no_argument,       0, 'F'},
        {"query",      no_argument,       0, 'Q'},
        {"remove",     no_argument,       0, 'R'},
        {"sync",       no_argument,       0, 'S'},
        {"deptest",    no_argument,       0, 'T'}, /* used by makepkg */
        {"upgrade",    no_argument,       0, 'U'},
        {"version",    no_argument,       0, 'V'},
        {"aur-only",   no_argument,       0, 'a'},
        {"help",       no_argument,       0, 'h'},
        {"test-colors",no_argument,       0, 't'},

        {"refresh",    no_argument,       0, OP_REFRESH},
        {"sysupgrade", no_argument,       0, OP_SYSUPGRADE},
        {"search",     no_argument,       0, OP_SEARCH},
        {"cachedir",   required_argument, 0, OP_CACHEDIR},
        {"colors",     required_argument, 0, OP_COLORS},
        {"config",     required_argument, 0, OP_CONFIG},
        {"theme",      required_argument, 0, OP_THEME},
        {"sudo",       required_argument, 0, OP_SUDO},
        {"use-git",    required_argument, 0, OP_USEGIT},
        {"quiet",      no_argument,       0, OP_QUIET},
        {"debug",      no_argument,       0, OP_DEBUG},
        {0,0,0,0}
    };

    /* parse operation */
    while ((opt = getopt_long(argc, argv, optstring, opts, &option_index)) != -1) {
        if (opt == 0) {
            continue;
        } else if (opt == '?') {
            return 1;
        }
        parsearg_op(opt, 0);
    }

    if(op.op == 0) {
		log_printf(LOG_ERROR, "only one operation may be used at a time\n");
		return 1;
    }

    if (op.version) {
        fmt::println("TabAUR version {}, branch {}", VERSION, BRANCH);
        exit(0);
    }
    if(op.help) {
		usage(op.op);
		exit(1);
    }
        
    /* parse all other options */
	optind = 1;
	while((opt = getopt_long(argc, argv, optstring, opts, &option_index)) != -1) {
		if(opt == 0) {
			continue;
		} else if(opt == '?') {
			/* this should have failed during first pass already */
			return 1;
		} else if(parsearg_op(opt, 1) == 0) {
			/* opt is an operation */
			continue;
		}

		switch(op.op) {
            case OP_SYNC:
                result = parsearg_sync(opt);
                break;
            case OP_QUERY:
                result = parsearg_query(opt);
                break;
            default:
				result = 1;
				break;
        }

        if(result == 0)
		continue;

        /* fall back to global options */
	    result = parsearg_global(opt);
	    if(result != 0) {
		    if(result == 1) {
		    /* global option parsing failed, abort */
			    if(opt < 1000) {
				    log_printf(LOG_ERROR, "invalid option '-{}'\n", opt);
			    } else {
				    log_printf(LOG_ERROR, "invalid option '--{}'\n", opts[option_index].name);
			    }
		    }
            return 1;
	    }       
    }

    while(optind < argc) {
		/* add the target to our target array */
        alpm_list_t *taur_targets_get = taur_targets.get();
        (void)taur_targets.release();
        alpm_list_append_strdup(&taur_targets_get, argv[optind]);
		taur_targets = make_list_smart_deleter(taur_targets_get);
		optind++;
	}
    
    return 0;
}

// main
int main(int argc, char* argv[]) {
    config = std::make_unique<Config>();

    configfile = (getConfigDir() + "/config.toml");
    themefile = (getConfigDir() + "/theme.toml");

    if (parseargs(argc, argv))
        return 1;

    // this will always be false
    // i just want to feel good about having this check
    config->init(configfile, themefile);

    fmt::disable_colors = config->colors == 0;

    if(op.test_colors) {
        test_colors();
        return 0;
    }

    signal(SIGINT, &interruptHandler);

    // the code you're likely interested in
    backend = std::make_unique<TaurBackend>(*config);

    if (op.requires_root && geteuid() != 0) {
        log_printf(LOG_ERROR, "You need to be root to do this.\n");
        return 1;
    } else if (!op.requires_root && geteuid() == 0) {
        log_printf(LOG_ERROR, "You are trying to run TabAUR as root when you don't need it.\n");
        return 1;
    }

    switch (op.op) {
    case OP_SYNC:
        return (installPkg(taur_targets ? string((const char *)(taur_targets->data)) : "")) ? 0 : 1;
    case OP_REM:
        return (removePkg(taur_targets ? string((const char *)(taur_targets->data)) : "")) ? 0 : 1;
    case OP_QUERY:
        return (queryPkgs()) ? 0 : 1;
    case OP_SYSUPGRADE:
        return (updateAll()) ? 0 : 1;
    case OP_PACMAN:
        // we are gonna use pacman to other ops than -S,-R,-Q
        return execPacman(argc, argv);
    }

    return 3;
}
