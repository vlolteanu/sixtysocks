#include <nspr.h>
#include <nss.h>
#include <ssl.h>
#include "tlsexception.hh"
#include "tlslibrary.hh"

TLSLibrary::NSPRLibrary::~NSPRLibrary()
{
	SSL_ClearSessionCache(); // might return error
	PR_Cleanup(); // might return error
}

TLSLibrary::NSSLibrary::NSSLibrary()
{
	SECStatus status = NSS_NoDB_Init(NULL);
	if (status != SECSuccess)
		throw TLSException();
}

TLSLibrary::NSSLibrary::NSSLibrary(const std::string &configDir)
{
	SECStatus status = NSS_Init(configDir.c_str());
	if (status != SECSuccess)
		throw TLSException();
}

TLSLibrary::NSSLibrary::~NSSLibrary()
{
	NSS_Shutdown(); // might return error
}

static void SECStatusCheck(SECStatus status)
{
	if (status != SECSuccess)
		throw TLSException();
}

static void init()
{
	SECStatusCheck(SSL_OptionSetDefault(SSL_ENABLE_FDX, PR_TRUE));
	SECStatusCheck(SSL_OptionSetDefault(SSL_ENABLE_SESSION_TICKETS, PR_TRUE));
	SECStatusCheck(SSL_OptionSetDefault(SSL_ENABLE_FALSE_START, PR_TRUE));
}

TLSLibrary::TLSLibrary()
{
	init();
}

TLSLibrary::TLSLibrary(const std::string &configDir)
	: NSSLibrary(configDir)
{
	init();
}
