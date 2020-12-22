#include "propd.h"
#include "PropertyServer.h"

void propd_entry(void)
{
	PropertyServer propsrv;
	propsrv.Entry();
}
