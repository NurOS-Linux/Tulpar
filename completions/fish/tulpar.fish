# SPDX-License-Identifier: GPL-3.0-only

set -l tulpar_commands install remove uninstall upgrade search list info \
    show verify orphans autoremove hold unhold history audit log repo \
    download key

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

complete -c tulpar -n "__fish_seen_subcommand_from repo" \
    -a "add remove list update"
complete -c tulpar -n "__fish_seen_subcommand_from key" \
    -a "add list"

complete -c tulpar -l dest -s d -d "alternate root" -r -a "(__fish_complete_directories)"
complete -c tulpar -l yes -s y -d "assume yes"
complete -c tulpar -l json -s j -d "json output"
complete -c tulpar -l quiet -s q -d "suppress informational output"
complete -c tulpar -l verbose -d "enable debug output"
complete -c tulpar -l require-signature -d "reject unsigned packages"
complete -c tulpar -l sign -d "detached signature path" -r
complete -c tulpar -l provider -d "force a provider for a dependency (name=package)" -r
complete -c tulpar -l exclude -d "skip a package during a full upgrade" -r
complete -c tulpar -s h -l help -d "show usage"
complete -c tulpar -s V -l version -d "show version"
