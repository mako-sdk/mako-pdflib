//
// makodistillercmd.cpp
// Copyright (C) 2021 Global Graphics Software Ltd. All rights reserved
//
// Simple example application that uses IDistiller to distill PostScript
// using options set from an argument file, where all supported options
// are compatible with PDF Library's testpdflibcmd application.
//

// std::filesystem is used for adding multiple fonts with -fa but
// requires c++17.  For earlier versions it can be disabled, but
// only file names without wildcards may be used when adding fonts.
#define WANT_STD_FILESYSTEM 1

#ifndef WANT_STD_FILESYSTEM
#define WANT_STD_FILESYSTEM 0
#endif

#include <exception>
#include <iostream>
#include <map>
#include <filesystem>
#include <jawsmako/jawsmako.h>
#include <jawsmako/distiller.h>

using namespace JawsMako;
using namespace EDL;

#if WANT_STD_FILESYSTEM
namespace fs = std::filesystem;
#endif

typedef std::pair<U8String, U8String> DistillerParam;
typedef std::vector<DistillerParam> DistillerParams;
typedef std::map<U8String, DistillerParam> ParamMap;

static void usage()
{
    std::wcout << L"================================================================" << std::endl;
    std::wcout << L"(C) Copyright 2021 Global Graphics Software Ltd." << std::endl;
    std::wcout << L"All Rights Reserved." << std::endl;
    std::wcout << L"================================================================" << std::endl;
    std::wcout << std::endl;
    std::wcout << L"Usage: makodistillercmd <arg_file>" << std::endl;
    std::wcout << std::endl;
    std::wcout << L"Options" << std::endl;
    std::wcout << L"-------" << std::endl;
    std::wcout << std::endl;
    std::wcout << L" Mode switches:" << std::endl;
    std::wcout << L"  -d           : distiller mode, convert PS files to PDF (default)" << std::endl;
    std::wcout << std::endl;
    std::wcout << L" Distill (PDF output) mode options:" << std::endl;
    std::wcout << L"  -dc...       : Colour image options (see below)" << std::endl;
    std::wcout << L"  -dd<dpi>     : resolution of the whole file (default 72)" << std::endl;
    std::wcout << L"  -dfe         : embed fonts" << std::endl;
    std::wcout << L"  -dfs         : subset fonts" << std::endl;
    std::wcout << L"  -dg...       : Greyscale image options (see below)" << std::endl;
    std::wcout << L"  -dm...       : Monochrome image options (see below)" << std::endl;
    std::wcout << L"  -dP<format>  : PDF file format (for example: -dP1.3 or -dP1.4)" << std::endl;
    std::wcout << L"  -dta         : Apply transfer functions" << std::endl;
    std::wcout << L"  -dtp         : Preserve transfer functions" << std::endl;
    std::wcout << L"  -dtr         : Remove transfer functions" << std::endl;
    std::wcout << L"  -dz          : Flate/Zip compress text" << std::endl;
    std::wcout << L"  -dZ<option>  : PDF1.5 object compression; option can be None, Tags or All" << std::endl;
    std::wcout << L"                 -dZ is equivalent to -dZAll; no -dZ means -dZNone" << std::endl;
    std::wcout << std::endl;
    std::wcout << L" Distill (PDF output) mode image options:" << std::endl;
    std::wcout << L"  -dc*         : colour image options" << std::endl;
    std::wcout << L"  -dg*         : greyscale image options" << std::endl;
    std::wcout << L"  -dm*         : monochrome image options" << std::endl;
    std::wcout << L"    where * is one or more of the following:" << std::endl;
    std::wcout << L"          A          = Auto compression" << std::endl;
    std::wcout << L"                       (NB: for colour/greyscale only, set JPEG options" << std::endl;
    std::wcout << L"                       with l/m/h/q switches)" << std::endl;
    std::wcout << L"          f          = Flate/Zip compression" << std::endl;
    std::wcout << L"          p          = Flate/Zip with Predictor compression" << std::endl;
    std::wcout << L"          l          = JPEG low compression (QFactor 0.1)" << std::endl;
    std::wcout << L"          m          = JPEG medium compression (QFactor 0.5)" << std::endl;
    std::wcout << L"          h          = JPEG high compression (QFactor 1.3)" << std::endl;
    std::wcout << L"          q<QFactor> = JPEG compression using specified QFactor" << std::endl;
    std::wcout << L"                       (NB: JPEG is for colour/greyscale only)" << std::endl;
    std::wcout << L"          c          = CCITT compression (monochrome only)" << std::endl;
    std::wcout << std::endl;
    std::wcout << L" Font-related options:" << std::endl;
    std::wcout << L"  -fp<fontpath>: specifies the font directory (must occur BEFORE" << std::endl;
    std::wcout << L"                 -fa or -fr if they're used).  If you use this," << std::endl;
    std::wcout << L"                 remember to put the Font\\*.* files in the specified" << std::endl;
    std::wcout << L"                 directory! (same as the -Pf option)" << std::endl;
    std::wcout << L"  -fa<filename>: adds the font filename so that Mako can use it," << std::endl;
    std::wcout << L"                 the filename can contain wildcards" << std::endl;
    std::wcout << L"  -ff<filename>: lists the names of the fonts that are available" << std::endl;
    std::wcout << L"                 in the specified font file" << std::endl;
    std::wcout << L"  -fr<fontname>: removes the named font from Mako (this switch" << std::endl;
    std::wcout << L"                 may be repeated if necessary)" << std::endl;
    std::wcout << std::endl;
    std::wcout << L" Path options:" << std::endl;
    std::wcout << L"  -Pf<fontpath> : sets the path where font files can be found (same as -fp)." << std::endl;
    std::wcout << L"  -Pr<respath>  : sets the path where the external resource files are found." << std::endl;
    std::wcout << std::endl;
    std::wcout << L" PostScript injection:" << std::endl;
    std::wcout << L"  -J<when><type><source><data> :" << std::endl;
    std::wcout << L"       where <when> is p for prolog or e for epilog" << std::endl;
    std::wcout << L"       and <type> is d for PDF output" << std::endl;
    std::wcout << L"       and <source> is f for a filename (data is a filename)" << std::endl;
    std::wcout << L"                    or c for command line (data is PostScript code)" << std::endl;
    std::wcout << L"       and <data> specifies PS code (literal or a filename) to be injected" << std::endl;
    std::wcout << L"           into the stream fed to the interpeter at the specified point for" << std::endl;
    std::wcout << L"           the specified type of output.  Implicit newline characters are" << std::endl;
    std::wcout << L"           added to the start and end of the data if <source> is c." << std::endl;
    std::wcout << L"    Example: '-Jpdf prolog.ps' would prefix all jobs producing PDF output" << std::endl;
    std::wcout << L"             with the PostScript code contained within the file 'prolog.ps'" << std::endl;
    std::wcout << L"    Example: '-Jedcshowpage' would perform an extra 'showpage' after all jobs" << std::endl;
    std::wcout << L"             producing PDF output." << std::endl;
    std::wcout << L"    Note that at most only one of each of the four -J<when><type> options" << std::endl;
    std::wcout << L"    should be used." << std::endl;
    std::wcout << std::endl;
    std::wcout << L" Miscellaneous options:" << std::endl;
    std::wcout << L"  -o<filename> : overrides the default output file name" << std::endl;
    std::wcout << L"  -i<options>  : passes the <options> string verbatim as extra options." << std::endl;
    std::wcout << L"                 <options> is a semicolon-separated list of key=value pairs." << std::endl;
    std::wcout << L"                 Note: only ONE -i argument can be supplied and it should be" << std::endl;
    std::wcout << L"                        the first argument on the command line." << std::endl;
    std::wcout << L"                 Valid options are:" << std::endl;
    std::wcout << L"                        defaultpanosestyle" << std::endl;
    std::wcout << L"                        panosedb" << std::endl;
    std::wcout << std::endl;
    std::wcout << L"  -h or -?     : this usage information" << std::endl;
    std::wcout << std::endl;
    std::wcout << std::endl;
    std::wcout << L"Example:" << std::endl;
    std::wcout << L"--------" << std::endl;
    std::wcout << L"makodistillercmd \"dist.args\"" << std::endl;
    std::wcout << std::endl;
    std::wcout << L"where dist.args is the configuration file for this particular" << std::endl;
    std::wcout << L"instance of distiller." << std::endl;
    std::wcout << std::endl;
    std::wcout << L"Sample dist.args" << std::endl;
    std::wcout << L"----------------" << std::endl;
    std::wcout << L"-d" << std::endl;
    std::wcout << L"test1.ps" << std::endl;
    std::wcout << L"test2.ps" << std::endl;
}

static void progressFunc(void *priv, float progress)
{
    uint32 *lastProgress = (uint32 *) priv;
    if (!lastProgress)
        return;
    
    uint32 iProgress = (uint32) (progress * 100);
    if (iProgress - *lastProgress >= 25)
    {
        std::wcout << L"\t" << iProgress << L"%..." << std::flush;

        *lastProgress = iProgress;
    }
}

static void setDistillerParameters(IDistillerPtr &distiller, DistillerParams &params)
{
    for (size_t i = 0; i < params.size(); i++)
    {
        // Enabling this will output the converted parameters for IDistiller.
#if 0
        std::cout << params[i].first << "=" << params[i].second << std::endl;
#endif
        distiller->setParameter(params[i].first, params[i].second);
    }
}

static bool pushParam(const char *line, size_t len, size_t need, ParamMap &paramMap, DistillerParams &params)
{
    if (len < need)
    {
        return false;
    }

    // Look for the option in the map
    ParamMap::iterator iter = paramMap.find(U8String(line, need));
    if (iter == paramMap.end())
    {
        return false;
    }

    // A valid option, push it.
    DistillerParam param = iter->second;
    if (param.second.length() == 0)
    {
        // Where there is no value defined we use the rest of the line.
        param.second = line + need;
    }
    else if (len != need)
    {
        // If there is a value defined we expect the line to be the key.
        return false;
    }
    params.push_back(param);
    return true;
}

#if WANT_STD_FILESYSTEM
static bool patternMatch(const char *pat, const char *str)
{
    switch (*pat)
    {
        case '\0':
            return *str == '\0';

        case '?':
            return *str && patternMatch(pat + 1, str + 1);

        case '*':
            return patternMatch(pat + 1, str) ||                   // 0 character match
                (*str != '\0' && patternMatch(pat, str + 1));   // or 1 character match

        default:
            return *pat == *str &&                                 // this char matches the current char of pattern
                patternMatch(pat + 1, str + 1);                 // next char matches the next char of pattern
    }
}

static U8String wstringToU8String(const std::wstring &wString)
{
    return StringToU8String(String(wString));
}
#endif

// Simple function to gather a list of file names.
// inPath may be a file or directory name.
// Note that wildcards are only supported in file names, e.g
// C:\fontdir\*.otf, but not C:\font*\font.otf
static bool getFileNames(const U8String &inPath, CU8StringVect &fileNames)
{
#if WANT_STD_FILESYSTEM
    fs::path p(inPath);
    std::error_code err;

    U8String pattern = "*";

    if (!fs::exists(p, err))
    {
        if (p.has_parent_path())
        {
            // Check the parent path.
            // If it exists we'll accept wildcards in file names.
            p = p.parent_path();
            if (!fs::exists(p, err))
            {
                return false;
            }
        }
        else
        {
            // No parent, use the current directory.
            p = fs::current_path();
        }
        p = fs::canonical(p);

        // Set the pattern to use when iterating the directory.
        pattern = inPath;

        size_t pos = pattern.find_last_of(fs::path::preferred_separator);
        if (pos != U8String::npos)
        {
            pattern = inPath.substr(pos + 1);
        }
    }
    else
    {
        p = fs::canonical(p);
    }

    if (fs::is_directory(p))
    {
        // Iterate the directory
        for (auto &ip : fs::recursive_directory_iterator(p))
        {
            if (!fs::is_directory(ip))
            {
                U8String fileName = wstringToU8String(ip.path().filename().wstring());

                // Do simple pattern matching on the file name
                if (patternMatch(pattern.c_str(), fileName.c_str()))
                {
                    // Add the file to the list
                    fileNames.append(wstringToU8String(ip.path().wstring()));
                }
            }
        }
    }
    else
    {
        fileNames.append(wstringToU8String(p.wstring()));
    }
#else
    fileNames.append(inPath);
#endif
    return true;
}

static bool pushPathParam(const char *line, size_t len, size_t need, ParamMap &paramMap, DistillerParams &params)
{
    if (len < need)
    {
        return false;
    }

    U8String pathOpt(line, need);
    U8String fullPath = line + need;

    // Look for the option in the map
    ParamMap::iterator iter = paramMap.find(pathOpt);
    if (iter == paramMap.end())
    {
        return false;
    }

    // A valid option, push it.

#if WANT_STD_FILESYSTEM
    fs::path p(fullPath);
    if (p.is_relative())
    {
        // For paths we don't want any relative paths.
        // Convert and push the canonical path.
        p = fs::canonical(p);

        fullPath = wstringToU8String(p.wstring());
    }
#endif

    DistillerParam param = iter->second;
    param.second = fullPath;
    params.push_back(param);
    return true;
}

static bool processImageOptions(const char *line, size_t len, ParamMap &paramMap, DistillerParams &params)
{
    if (len < 2)
    {
        return false;
    }

    U8String type(line, 2);
    bool added = false;
    for (size_t i = 2; i < len; i++)
    {
        U8String param;
        switch (line[i])
        {
            case 'q':
                // For the QFactor we need to do extra.
                if (!pushParam(line, len, 3, paramMap, params))
                {
                    continue;
                }

                // Indicate that we are doing user JPEG quality and DCT compression.
                param = type + 'u';
                if (!pushParam(param.c_str(), 3, 3, paramMap, params))
                {
                    continue;
                }
                param = type + 'd';
                break;

            case 'l':
            case 'm':
            case 'h':
            case 'u':
                // JPEG options imply DCT compression.
                if (!pushParam(line, 3, 3, paramMap, params))
                {
                    continue;
                }

                // Indicate that we are doing DCT compression.
                param = type + 'd';
                break;

            default:
                param = type + line[i];
        }

        if (pushParam(param.c_str(), 3, 3, paramMap, params))
        {
            added = true;
        }
    }
    return added;
}

static bool processDistillOptions(const char *line, size_t len, ParamMap &paramMap, DistillerParams &params)
{
    // All parameters that set a value are two bytes (e.g -dP1.7)
    // We assume that the rest of the line is the value.
    if (pushParam(line, len, 2, paramMap, params))
    {
        return true;
    }
    
    // Check if it's an image option, where more than
    // one option can be set from a single line.
    if (processImageOptions(line, len, paramMap, params))
    {
        return true;
    }

    // The rest set no value, so we can use the entire line (e.g -dz).
    return pushParam(line, len, len, paramMap, params);
}

static bool processFontOptions(const char *line, size_t len, ParamMap &paramMap, DistillerParams &params, IDistillerPtr &distiller, CU8StringVect &names)
{
    // First handle the parameters that set a value of two bytes (e.g -fp)
    // We assume that the rest of the line is the value.
    switch (line[1])
    {
        case 'p':
            return pushPathParam(line, len, 2, paramMap, params);

        case 'r':
            // Remove the font, but first we need to set any pushed parameters
            // in case we are using a custom font/resource device.
            setDistillerParameters(distiller, params);

            // Clear the parameters, we don't need to set them again.
            params.clear();

            distiller->removeFont(line + 2);
            break;

        case 'a':
        {
            CU8StringVect fileNames;
            getFileNames(line + 2, fileNames);

            // Add the fonts, but first we need to set any pushed parameters
            // in case we are using a custom font/resource device.
            setDistillerParameters(distiller, params);

            // Clear the parameters, we don't need to set them again.
            params.clear();

            distiller->addFonts(fileNames);
            break;
        }

        case 'f':
            // Return list font of font names
            distiller->getFontNames(line + 2, names);
            break;

        default:
            return false;
    }

    return true;
}

static bool processPrologEpilogOptions(const char *line, size_t len, ParamMap &paramMap, DistillerParams &params)
{
    if (line[3] == 'f')
    {
        return pushPathParam(line, len, 4, paramMap, params);
    }
    return pushParam(line, len, 4, paramMap, params);
}

static bool processExtraOptions(const char *line, size_t len, ParamMap &paramMap, DistillerParams &params)
{
    EDLSysStringIStream ss(line + 1);
    U8String token;

    bool added = false;

    while (std::getline(ss, token, ';'))
    {
        size_t pos = token.find("=");
        if (pos == U8String::npos)
        {
            // Ignore
            continue;
        }

        // Remove the =
        token.erase(pos, 1);

        // Push
        U8String param = line[0] + token;
        if (param.find("ipanosedb") == 0)
        {
            if (pushPathParam(param.c_str(), param.length(), pos + 1, paramMap, params))
            {
                added = true;
                continue;
            }
        }

        if (pushParam(param.c_str(), param.length(), pos + 1, paramMap, params))
        {
            added = true;
        }
    }
    return added;
}

#ifdef _WIN32
int wmain(int argc, wchar_t *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    try
    {
        if (argc != 2)
        {
            // Only a single arg file supported
            usage();
            return 1;
        }

        std::ifstream argFile(argv[1]);
        if (!argFile.is_open())
        {
            std::wcerr << L"Error opening file : " << argv[1] << std::endl;
            return 1;
        }

        // Create IJawsMako instance
        IJawsMakoPtr jawsMako = IJawsMako::create();

        // Create a progress monitor
        uint32              progress = 0;
        IAbortPtr           abort = IAbort::create();
        IProgressTickPtr    progressTick = IProgressTick::create((IProgressTick::FloatProgressCallbackFunc) progressFunc, &progress);
        IProgressMonitorPtr progressMonitor = IProgressMonitor::create(progressTick, abort);

        // Simple options and their IDistiller equivalents
        ParamMap paramMap;
        paramMap["dcA"] = DistillerParam("colorimagecompression", "auto");
        paramMap["dcd"] = DistillerParam("colorimagecompression", "dct");
        paramMap["dcf"] = DistillerParam("colorimagecompression", "flate");
        paramMap["dcp"] = DistillerParam("colorimagecompression", "flatepredict");
        paramMap["dcl"] = DistillerParam("colorjpegquality", "low");
        paramMap["dcm"] = DistillerParam("colorjpegquality", "medium");
        paramMap["dch"] = DistillerParam("colorjpegquality", "high");
        paramMap["dcu"] = DistillerParam("colorjpegquality", "user");
        paramMap["dcq"] = DistillerParam("colorqfactor", "");
        paramMap["dgA"] = DistillerParam("grayimagecompression", "auto");
        paramMap["dgd"] = DistillerParam("grayimagecompression", "dct");
        paramMap["dgf"] = DistillerParam("grayimagecompression", "flate");
        paramMap["dgp"] = DistillerParam("grayimagecompression", "flatepredict");
        paramMap["dgl"] = DistillerParam("grayjpegquality", "low");
        paramMap["dgm"] = DistillerParam("grayjpegquality", "medium");
        paramMap["dgh"] = DistillerParam("grayjpegquality", "high");
        paramMap["dgu"] = DistillerParam("grayjpegquality", "user");
        paramMap["dgq"] = DistillerParam("grayqfactor", "");
        paramMap["dmf"] = DistillerParam("monoimagecompression", "flate");
        paramMap["dmp"] = DistillerParam("monoimagecompression", "flatepredict");
        paramMap["dmc"] = DistillerParam("monoimagecompression", "ccitt");
        paramMap["dd"] = DistillerParam("resolution", "");
        paramMap["dfe"] = DistillerParam("embedfonts", "true");
        paramMap["dfs"] = DistillerParam("subsetfonts", "true");
        paramMap["dP"] = DistillerParam("pdfversion", "");
        paramMap["dta"] = DistillerParam("transfers", "apply");
        paramMap["dtp"] = DistillerParam("transfers", "preserve");
        paramMap["dtr"] = DistillerParam("transfers", "remove");
        paramMap["dz"] = DistillerParam("compresspages", "true");
        paramMap["fp"] = DistillerParam("fontdevice", "");
        paramMap["idefaultpanosestyle"] = DistillerParam("defaultpanosestyle", "");
        paramMap["ipanosedb"] = DistillerParam("panose", "");
        paramMap["Jedc"] = DistillerParam("epilogcommand", "");
        paramMap["Jedf"] = DistillerParam("epilogfile", "");
        paramMap["Jpdc"] = DistillerParam("prologcommand", "");
        paramMap["Jpdf"] = DistillerParam("prologfile", "");
        paramMap["Pf"] = DistillerParam("fontdevice", "");
        paramMap["Pr"] = DistillerParam("resourcedevice", "");

        // The list of the distiller options pushed from the arg file.
        DistillerParams distillerParams;

        // The output path.
        U8String outputFilePath;

        // Create a distiller
        IDistillerPtr distiller = IDistiller::create(jawsMako);

        // Set default parameters
        distiller->setResolution(72.0f);
        distiller->setCompressPages(false);
        distiller->setSubsetFonts(false);
        distiller->setEmbedFonts(false);
        distiller->setColorImageCompression(IDistiller::eICNone);
        distiller->setGrayImageCompression(IDistiller::eICNone);
        distiller->setMonoImageCompression(IDistiller::eICNone);
        distiller->setTransfers(IDistiller::eTRRemove);

        std::wcout << std::endl;
        char line[1024];
        while (argFile.getline(line, sizeof(line)))
        {
            if (line[0] == '\0')
            {
                continue;
            }

            size_t len = strlen(line);
            if (len > 1 && line[0] == '-')
            {
                CU8StringVect fontNames;
                bool added = false;
                char *pline = line + 1;
                --len;

                switch (*pline)
                {
                    // Distill options
                    case 'd':
                        added = processDistillOptions(pline, len, paramMap, distillerParams);
                        break;

                    // Font options
                    case 'f':
                        added = processFontOptions(pline, len, paramMap, distillerParams, distiller, fontNames);
                        break;

                    // Extra options
                    case 'i':
                        added = processExtraOptions(pline, len, paramMap, distillerParams);
                        break;

                    // Prolog/Epilog options
                    case 'J':
                        added = processPrologEpilogOptions(pline, len, paramMap, distillerParams);
                        break;

                    // Path options
                    case 'P':
                        added = pushPathParam(pline, len, 2, paramMap, distillerParams);
                        break;

                    // Output path
                    case 'o':
                        outputFilePath = ++pline;
                        added = true;
                        break;

                    // Help
                    case 'h':
                    case '?':
                    default:
                        usage();
                        break;
                }
                if (added)
                {
                    std::wcout << L"Processing argument line from file : " << argv[1] << std::endl;
                    std::wcout << line << std::endl << std::endl;

                    for (uint32 i = 0; i < fontNames.size(); i++)
                    {
                        std::cout << fontNames[i] << std::endl;
                    }
                    std::wcout << std::endl << std::endl;
                    fontNames.clear();
                }
            }
            else
            {
                // Assume it's the input file if it doesn't begin with '-'.
                U8String inputFilePath = line;

                // Was an output path set?
                if (outputFilePath.length() == 0)
                {
                    // No, use the input
                    outputFilePath = inputFilePath + ".pdf";
                }

                // Set the distill parameters if any
                setDistillerParameters(distiller, distillerParams);

                std::cout << "Converting " << inputFilePath << " to " << outputFilePath << std::endl;

                // Distill
                distiller->distill(IInputStream::createFromFile(jawsMako, inputFilePath), IOutputStream::createToFile(jawsMako, outputFilePath), progressMonitor);

                std::wcout << std::endl << std::endl;
            }
        }

        argFile.close();

    }
    catch(IError &e)
    {
        String errorFormatString = getEDLErrorString(e.getErrorCode());
        std::wcerr << L"Exception thrown: " << e.getErrorDescription(errorFormatString) << std::endl;
#ifdef _WIN32
        // On windows, the return code allows larger numbers, and we can return the error code
        return e.getErrorCode();
#else
        // On other platforms, the exit code is masked to the low 8 bits. So here we just return
        // a fixed value.
        return 1;
#endif
    }
    catch(std::exception &e)
    {
        std::wcerr << L"std::exception thrown: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
