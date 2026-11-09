# SPDX-License-Identifier: GPL-3.0-only

set -l tulpar_commands install remove uninstall upgrade search list info \
    show verify orphans autoremove hold unhold history audit log repo \
    download key graph clean autoclean rollback undo owns files why mark \
    reinstall stats

complete -c tulpar -f

complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "install" -d "install one or more packages"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "remove uninstall" -d "remove an installed package"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "upgrade" -d "upgrade one or all installed packages"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "search" -d "search local and remote package indexes"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "list" -d "list installed packages"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "info show" -d "show full metadata for a package"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "verify" -d "verify installed package files"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "orphans autoremove" -d "list and remove orphaned packages"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "hold" -d "block a package from upgrade or removal"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "unhold" -d "release a held package"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "history audit log" -d "show past install/remove operations"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "repo" -d "manage configured repositories"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "download" -d "fetch a package without installing it"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "key" -d "manage the trusted signing keyring"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "graph" -d "export the installed package dependency graph as DOT"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "clean autoclean" -d "purge downloaded packages and metadata cache"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "rollback undo" -d "undo the most recent package operation"

complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "owns" -d "find the package that owns a file"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "files" -d "list the files of an installed package"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "why" -d "explain why a package is installed"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "mark" -d "mark a package as automatic or explicit"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "reinstall" -d "reinstall installed packages"
complete -c tulpar -n "not __fish_seen_subcommand_from $tulpar_commands" \
    -a "stats" -d "show package database statistics"

function __tulpar_dest_args
    set -l tokens (commandline -opc)
    for i in (seq 1 (count $tokens))
        switch $tokens[$i]
            case --dest -d
                if test (math $i + 1) -le (count $tokens)
                    echo --dest
                    echo $tokens[(math $i + 1)]
                end
                return
        end
    end
end

function __tulpar_installed
    tulpar list --names (__tulpar_dest_args) 2>/dev/null
end

function __tulpar_repos
    tulpar repo list --all 2>/dev/null | cut -f2
end

complete -c tulpar -n "__fish_seen_subcommand_from remove uninstall upgrade info show hold unhold files why mark reinstall verify" \
    -a "(__tulpar_installed)"
complete -c tulpar -n "__fish_seen_subcommand_from info show reinstall" \
    -a "(__fish_complete_suffix .apg)"
complete -c tulpar -n "__fish_seen_subcommand_from owns" -F

complete -c tulpar -n "__fish_seen_subcommand_from repo" \
    -a "add remove enable disable priority list update"
complete -c tulpar -n "__fish_seen_subcommand_from repo; and __fish_seen_subcommand_from remove enable disable priority" \
    -a "(__tulpar_repos)"
complete -c tulpar -n "__fish_seen_subcommand_from repo" -l user -d "use the per-user repository list"
complete -c tulpar -n "__fish_seen_subcommand_from repo" -l all -s a -d "include disabled repositories"
complete -c tulpar -n "__fish_seen_subcommand_from key" \
    -a "add list"

complete -c tulpar -l dest -s d -d "alternate root" -r -a "(__fish_complete_directories)"
complete -c tulpar -l yes -s y -d "assume yes"
complete -c tulpar -l json -s j -d "json output"
complete -c tulpar -l quiet -s q -d "suppress informational output"
complete -c tulpar -l verbose -d "enable debug output"
complete -c tulpar -l offline -d "never touch the network, use cached indexes"
complete -c tulpar -l color -x -a "auto always never" -d "colorize output"
complete -c tulpar -l require-signature -d "reject unsigned packages"
complete -c tulpar -l sign -d "detached signature path" -r
complete -c tulpar -l provider -d "force a provider for a dependency (name=package)" -r
complete -c tulpar -l nodeps -d "install exactly what was requested, skip dependency resolution"
complete -c tulpar -l dry-run -d "preview the transaction without writing anything to disk"
complete -c tulpar -l db -d "also check the package database for inconsistencies"
complete -c tulpar -l exclude -d "skip a package during a full upgrade" -r
complete -c tulpar -l installed -d "search installed packages only"
complete -c tulpar -l auto -d "mark as automatically installed"
complete -c tulpar -l explicit -d "mark as explicitly installed"
complete -c tulpar -l deps -d "list automatically installed packages only"
complete -c tulpar -l held -d "list held packages only"
complete -c tulpar -l upgradable -d "list packages with a newer version available"
complete -c tulpar -l names -d "print package names only"
complete -c tulpar -l package -s p -d "only entries of this package" -x -a "(__tulpar_installed)"
complete -c tulpar -l since -d "only entries since a time" -x -a "1h 12h 1d 7d 2w"
complete -c tulpar -l remote -d "search remote packages only"
complete -c tulpar -l output -s o -d "output path" -r
complete -c tulpar -s h -l help -d "show usage"
complete -c tulpar -s V -l version -d "show version"
