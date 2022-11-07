#include "pch.h"
#include "Importer.hpp"

namespace General
{
	namespace Models
	{
		Importer::Importer(const char* filename) : mFilename(filename ? (char*)malloc(strlen(filename) + 1) : nullptr) 
		{
			if (filename)
			{
				strcpy(mFilename, filename);
			}
		}

		Importer::~Importer()
		{
			if (mFilename)
			{
				free((void*)mFilename);
			}
		}

		inline const char* Importer::GetFilename() const { return mFilename; }
	}
}