// NurOS Ruzen42 2025 apg/package.h
// Last change: Dec 21
#include <stdlib.h>

package_metadata *
package_metadata_new(void)
{
	package_metadata *meta;

	meta = calloc(1, sizeof(*meta));
	if (!meta)
		return NULL;

	return meta;
}

package *
package_new(void)
{
    package *pkg;

    pkg = calloc(1, sizeof(*pkg));
    if (!pkg)
		return NULL;

    pkg->meta = package_metadata_new();
    if (!pkg->meta) {
        free(pkg);
        return NULL;
    }

    return pkg;
}

void
str_list_free(str_list *list)
{
	if (!list)
		return;

	free(list->items);
}

void
package_metadata_free(package_metadata *meta)
{
    if (!meta)
        return;

    free(meta->name);
    free(meta->version);
    free(meta->architecture);

    free(meta->maintainer);
    free(meta->license);
    free(meta->homepage);

    str_list_free(&meta->dependencies);
    str_list_free(&meta->conflicts);
    str_list_free(&meta->provides);
    str_list_free(&meta->replaces);
    str_list_free(&meta->tags);

    free(meta);
}

void
package_free(package *pkg)
{
    if (!pkg)
        return;

    package_metadata_free(pkg->meta);
    free(pkg->pkg_path);

    str_list_free(&pkg->package_files);

    free(pkg);
}

void
install_package(package *pkg)
{

}
