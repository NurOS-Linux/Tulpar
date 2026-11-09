// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <apg/package.h>

#include "why_chain.h"

struct node
{
    char *name;
    int parent;
};

static bool
is_explicit(struct db_handle *db, const char *name)
{
    struct package *pkg = db_get(db, name);
    if (!pkg)
        return false;
    bool by_hand = pkg->installed_by_hand;
    package_free(pkg);
    return by_hand;
}

static int
find_node(const struct node *nodes, int count, const char *name)
{
    for (int i = 0; i < count; i++)
        if (strcmp(nodes[i].name, name) == 0)
            return i;
    return -1;
}

void
why_free_names(char **names, int count)
{
    for (int i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

static char **
build_chain(const struct node *nodes, int leaf, int *count)
{
    int len = 0;
    for (int at = leaf; at >= 0; at = nodes[at].parent)
        len++;

    char **chain = calloc((size_t)len, sizeof(*chain));
    if (!chain)
        return NULL;

    int idx = 0;
    for (int at = leaf; at >= 0; at = nodes[at].parent)
    {
        chain[idx] = strdup(nodes[at].name);
        if (!chain[idx])
        {
            why_free_names(chain, idx);
            return NULL;
        }
        idx++;
    }
    *count = len;
    return chain;
}

char **
why_find_chain(struct db_handle *db, const char *name, int *count)
{
    *count = 0;

    struct node *nodes = NULL;
    int node_count = 0;
    int node_cap = 0;
    char **result = NULL;

    node_cap = 16;
    nodes = calloc((size_t)node_cap, sizeof(*nodes));
    if (!nodes)
        return NULL;
    nodes[0].name = strdup(name);
    nodes[0].parent = -1;
    node_count = 1;

    for (int head = 0; head < node_count && !result; head++)
    {
        if (head > 0 && is_explicit(db, nodes[head].name))
        {
            result = build_chain(nodes, head, count);
            break;
        }

        int dep_count = 0;
        char **dependents = db_get_dependents(db, nodes[head].name, &dep_count);
        for (int i = 0; dependents && i < dep_count; i++)
        {
            if (find_node(nodes, node_count, dependents[i]) >= 0)
                continue;
            if (node_count == node_cap)
            {
                int cap = node_cap * 2;
                struct node *tmp = realloc(nodes, (size_t)cap * sizeof(*nodes));
                if (!tmp)
                    break;
                nodes = tmp;
                node_cap = cap;
            }
            nodes[node_count].name = strdup(dependents[i]);
            nodes[node_count].parent = head;
            node_count++;
        }
        why_free_names(dependents, dep_count);
    }

    for (int i = 0; i < node_count; i++)
        free(nodes[i].name);
    free(nodes);
    return result;
}
