/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file configPageManager.cxx
 * @author drose
 * @date 2004-10-15
 */

#include "configPageManager.h"
#include "configDeclaration.h"
#include "configVariableBool.h"
#include "configVariableString.h"
#include "configPage.h"
#include "prcKeyRegistry.h"
#include "dSearchPath.h"
#include "executionEnvironment.h"
#include "config_prc.h"
#include "pfstream.h"
#include "pandaSystem.h"
#include "textEncoder.h"
#include "stringDecoder.h"

// This file is generated by ppremake.
#include "prc_parameters.h"

#include <set>

// Pick up the public key definitions.
#ifdef PRC_PUBLIC_KEYS_INCLUDE
#include PRC_PUBLIC_KEYS_INCLUDE
#endif

#include <algorithm>
#include <ctype.h>

#ifndef _WIN32
#include <dlfcn.h>
#endif

using std::string;

ConfigPageManager *ConfigPageManager::_global_ptr = nullptr;

/**
 * The constructor is private (actually, just protected, but only to avoid a
 * gcc compiler warning) because it should not be explicitly constructed.
 * There is only one ConfigPageManager, and it constructs itself.
 */
ConfigPageManager::
ConfigPageManager() {
  _next_page_seq = 1;
  _loaded_implicit = false;
  _currently_loading = false;
  _pages_sorted = true;

#ifdef PRC_PUBLIC_KEYS_INCLUDE
  // Record the public keys in the registry at startup time.
  PrcKeyRegistry::get_global_ptr()->record_keys(prc_pubkeys, num_prc_pubkeys);
#endif  // PRC_PUBLIC_KEYS_INCLUDE
}

/**
 * The ConfigPageManager destructor should never be called, because this is a
 * global object that is never freed.
 */
ConfigPageManager::
~ConfigPageManager() {
  prc_cat->error()
    << "Internal error--ConfigPageManager destructor called!\n";
}

/**
 * Searches the PRC_DIR and/or PRC_PATH directories for *.prc files and loads
 * them in as pages.
 *
 * This may be called after startup, to force the system to re-read all of the
 * implicit prc files.
 */
void ConfigPageManager::
reload_implicit_pages() {
// Don't read implicit pages on static builds (aka CIO production).
#ifndef LINK_ALL_STATIC
  if (_currently_loading) {
    // This is a recursion protector.  We can get recursion feedback between
    // config and notify, as each tries to use the other at construction.
    return;
  }
  _currently_loading = true;

  // First, remove all the previously-loaded pages.
  Pages::iterator pi;
  for (pi = _implicit_pages.begin(); pi != _implicit_pages.end(); ++pi) {
    delete (*pi);
  }
  _implicit_pages.clear();

#ifndef ANDROID
  // If we are running inside a deployed application, see if it exposes
  // information about how the PRC data should be initialized.
  struct BlobInfo {
    uint64_t blob_offset;
    uint64_t blob_size;
    uint16_t version;
    uint16_t num_pointers;
    uint16_t codepage;
    uint16_t flags;
    uint64_t reserved;
    const void *module_table;
    const char *prc_data;
    const char *default_prc_dir;
    const char *prc_dir_envvars;
    const char *prc_path_envvars;
    const char *prc_patterns;
    const char *prc_encrypted_patterns;
    const char *prc_encryption_key;
    const char *prc_executable_patterns;
    const char *prc_executable_args_envvar;
    const char *main_dir;
    const char *log_filename;
  };
#ifdef _WIN32
  const BlobInfo *blobinfo = (const BlobInfo *)GetProcAddress(GetModuleHandle(nullptr), "blobinfo");
#elif defined(RTLD_MAIN_ONLY)
  const BlobInfo *blobinfo = (const BlobInfo *)dlsym(RTLD_MAIN_ONLY, "blobinfo");
//#elif defined(RTLD_SELF)
//  const BlobInfo *blobinfo = (const BlobInfo *)dlsym(RTLD_SELF, "blobinfo");
#elif defined(__EMSCRIPTEN__)
  const BlobInfo *blobinfo = nullptr;
#else
  const BlobInfo *blobinfo = (const BlobInfo *)dlsym(dlopen(nullptr, RTLD_NOW), "blobinfo");
#endif
  if (blobinfo == nullptr) {
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
    // Clear the error flag.
    dlerror();
#endif
  } else if (blobinfo->version == 0 || blobinfo->num_pointers < 10) {
    blobinfo = nullptr;
  }

  if (blobinfo != nullptr) {
    if (blobinfo->num_pointers >= 11 && blobinfo->main_dir != nullptr) {
      ExecutionEnvironment::set_environment_variable("MAIN_DIR", blobinfo->main_dir);
    } else {
      // Make sure that py_panda.cxx won't override MAIN_DIR.
      ExecutionEnvironment::set_environment_variable("MAIN_DIR",
        ExecutionEnvironment::get_environment_variable("MAIN_DIR"));
    }
  }

  // PRC_PATTERNS lists one or more filename templates separated by spaces.
  // Pull them out and store them in _prc_patterns.
  _prc_patterns.clear();

#ifdef PRC_PATTERNS
  string prc_patterns = PRC_PATTERNS;
  if (blobinfo != nullptr && blobinfo->prc_patterns != nullptr) {
    prc_patterns = blobinfo->prc_patterns;
  }
  if (!prc_patterns.empty()) {
    vector_string pat_list;
    ConfigDeclaration::extract_words(prc_patterns, pat_list);
    _prc_patterns.reserve(pat_list.size());
    for (size_t i = 0; i < pat_list.size(); ++i) {
      GlobPattern glob(pat_list[i]);
#ifdef _WIN32
      // On windows, the file system is case-insensitive, so the pattern
      // should be too.
      glob.set_case_sensitive(false);
#endif  // WIN32
      _prc_patterns.push_back(glob);
    }
  }
#endif  // PRC_PATTERNS

  // Similarly for PRC_ENCRYPTED_PATTERNS.
  _prc_encrypted_patterns.clear();

#ifdef PRC_ENCRYPTED_PATTERNS
  string prc_encrypted_patterns = PRC_ENCRYPTED_PATTERNS;
  if (blobinfo != nullptr && blobinfo->prc_encrypted_patterns != nullptr) {
    prc_encrypted_patterns = blobinfo->prc_encrypted_patterns;
  }
  if (!prc_encrypted_patterns.empty()) {
    vector_string pat_list;
    ConfigDeclaration::extract_words(prc_encrypted_patterns, pat_list);
    _prc_encrypted_patterns.reserve(pat_list.size());
    for (size_t i = 0; i < pat_list.size(); ++i) {
      GlobPattern glob(pat_list[i]);
#ifdef _WIN32
      glob.set_case_sensitive(false);
#endif  // WIN32
      _prc_encrypted_patterns.push_back(glob);
    }
  }
#endif  // PRC_ENCRYPTED_PATTERNS

  // And again for PRC_EXECUTABLE_PATTERNS.
  _prc_executable_patterns.clear();

#ifdef PRC_EXECUTABLE_PATTERNS
  string prc_executable_patterns = PRC_EXECUTABLE_PATTERNS;
  if (blobinfo != nullptr && blobinfo->prc_executable_patterns != nullptr) {
    prc_executable_patterns = blobinfo->prc_executable_patterns;
  }
  if (!prc_executable_patterns.empty()) {
    vector_string pat_list;
    ConfigDeclaration::extract_words(prc_executable_patterns, pat_list);
    _prc_executable_patterns.reserve(pat_list.size());
    for (size_t i = 0; i < pat_list.size(); ++i) {
      GlobPattern glob(pat_list[i]);
#ifdef _WIN32
      glob.set_case_sensitive(false);
#endif  // WIN32
      _prc_executable_patterns.push_back(glob);
    }
  }
#endif  // PRC_EXECUTABLE_PATTERNS

  // Now build up the search path for .prc files.
  _search_path.clear();

#ifdef PRC_DIR_ENVVARS
  // PRC_DIR_ENVVARS lists one or more environment variables separated by
  // spaces.  Pull them out, and each of those contains the name of a single
  // directory to search.  Add it to the search path.
  string prc_dir_envvars = PRC_DIR_ENVVARS;
  if (blobinfo != nullptr && blobinfo->prc_dir_envvars != nullptr) {
    prc_dir_envvars = blobinfo->prc_dir_envvars;
  }
  if (!prc_dir_envvars.empty()) {
    vector_string prc_dir_envvar_list;
    ConfigDeclaration::extract_words(prc_dir_envvars, prc_dir_envvar_list);
    for (size_t i = 0; i < prc_dir_envvar_list.size(); ++i) {
      string prc_dir = ExecutionEnvironment::get_environment_variable(prc_dir_envvar_list[i]);
      if (!prc_dir.empty()) {
        Filename prc_dir_filename = Filename::from_os_specific(prc_dir);
        prc_dir_filename.make_true_case();
        if (scan_auto_prc_dir(prc_dir_filename)) {
          _search_path.append_directory(prc_dir_filename);
        }
      }
    }
  }
#endif  // PRC_DIR_ENVVARS

#ifdef PRC_PATH_ENVVARS
  // PRC_PATH_ENVVARS lists one or more environment variables separated by
  // spaces.  Pull them out, and then each one of those contains a list of
  // directories to search.  Add each of those to the search path.
  string prc_path_envvars = PRC_PATH_ENVVARS;
  if (blobinfo != nullptr && blobinfo->prc_path_envvars != nullptr) {
    prc_path_envvars = blobinfo->prc_path_envvars;
  }
  if (!prc_path_envvars.empty()) {
    vector_string prc_path_envvar_list;
    ConfigDeclaration::extract_words(prc_path_envvars, prc_path_envvar_list);
    for (size_t i = 0; i < prc_path_envvar_list.size(); ++i) {
      string path = ExecutionEnvironment::get_environment_variable(prc_path_envvar_list[i]);
      size_t p = 0;
      while (p < path.length()) {
        size_t q = path.find_first_of(DEFAULT_PATHSEP, p);
        if (q == string::npos) {
          q = path.length();
        }
        Filename prc_dir_filename = Filename::from_os_specific(path.substr(p, q - p));
        prc_dir_filename.make_true_case();
        if (scan_auto_prc_dir(prc_dir_filename)) {
          _search_path.append_directory(prc_dir_filename);
        }
        p = q + 1;
      }
    }
  }
#endif  // PRC_PATH_ENVVARS

#ifdef PRC_PATH2_ENVVARS
/*
 * PRC_PATH2_ENVVARS is a special variable that is rarely used; it exists
 * primarily to support the Cygwin-based "ctattach" tools used by the Walt
 * Disney VR Studio.  This defines a set of environment variable(s) that
 * define a search path, as above; except that the directory names on these
 * search paths are Panda-style filenames, not Windows-style filenames; and
 * the path separator is always a space character, regardless of
 * DEFAULT_PATHSEP.
 */
  string prc_path2_envvars = PRC_PATH2_ENVVARS;
  if (!prc_path2_envvars.empty() && blobinfo == nullptr) {
    vector_string prc_path_envvar_list;
    ConfigDeclaration::extract_words(prc_path2_envvars, prc_path_envvar_list);
    for (size_t i = 0; i < prc_path_envvar_list.size(); ++i) {
      string path = ExecutionEnvironment::get_environment_variable(prc_path_envvar_list[i]);
      size_t p = 0;
      while (p < path.length()) {
        size_t q = path.find_first_of(' ', p);
        if (q == string::npos) {
          q = path.length();
        }
        Filename prc_dir_filename = path.substr(p, q - p);
        if (scan_auto_prc_dir(prc_dir_filename)) {
          _search_path.append_directory(prc_dir_filename);
        }
        p = q + 1;
      }
    }
  }
#endif  // PRC_PATH2_ENVVARS

#ifdef DEFAULT_PRC_DIR
  if (_search_path.is_empty()) {
    // If nothing's on the search path (PRC_DIR and PRC_PATH were not
    // defined), then use the DEFAULT_PRC_DIR.
    string default_prc_dir = DEFAULT_PRC_DIR;
    if (blobinfo != nullptr && blobinfo->default_prc_dir != nullptr) {
      default_prc_dir = blobinfo->default_prc_dir;
    }
    if (!default_prc_dir.empty()) {
      // It's already from-os-specific by ppremake.
      Filename prc_dir_filename = default_prc_dir;
      if (scan_auto_prc_dir(prc_dir_filename)) {
        _search_path.append_directory(prc_dir_filename);
      }
    }
  }
#endif  // DEFAULT_PRC_DIR

  // Now find all of the *.prc files (or whatever matches PRC_PATTERNS) on the
  // path.
  ConfigFiles config_files;

  // Use a set to ensure that we only visit each directory once, even if it
  // appears multiple times (under different aliases!) in the path.
  std::set<Filename> unique_dirnames;

  // We walk through the list of directories in forward order, so that the
  // most important directories are visited first.
  for (size_t di = 0; di < _search_path.get_num_directories(); ++di) {
    const Filename &directory = _search_path.get_directory(di);
    if (directory.is_directory()) {
      Filename canonical(directory, ".");
      canonical.make_canonical();
      if (unique_dirnames.insert(canonical).second) {
        vector_string files;
        directory.scan_directory(files);

        // We walk through the directory's list of files in reverse
        // alphabetical order, because for historical reasons, the most
        // important file within a directory is the alphabetically last file
        // of that directory, and we still want to visit the most important
        // files first.
        vector_string::reverse_iterator fi;
        for (fi = files.rbegin(); fi != files.rend(); ++fi) {
          int file_flags = 0;
          Globs::const_iterator gi;
          for (gi = _prc_patterns.begin();
               gi != _prc_patterns.end();
               ++gi) {
            if ((*gi).matches(*fi)) {
              file_flags |= FF_read;
              break;
            }
          }
          for (gi = _prc_encrypted_patterns.begin();
               gi != _prc_encrypted_patterns.end();
               ++gi) {
            if ((*gi).matches(*fi)) {
              file_flags |= FF_read | FF_decrypt;
              break;
            }
          }
          for (gi = _prc_executable_patterns.begin();
               gi != _prc_executable_patterns.end();
               ++gi) {
            if ((*gi).matches(*fi)) {
              file_flags |= FF_execute;
              break;
            }
          }
          if (file_flags != 0) {
            ConfigFile file;
            file._file_flags = file_flags;
            file._filename = Filename(directory, (*fi));
            config_files.push_back(file);
          }
        }
      }
    }
  }

  int i = 1;

  // If prc_data is predefined, we load it as an implicit page.
  if (blobinfo != nullptr && blobinfo->prc_data != nullptr) {
    ConfigPage *page = new ConfigPage("builtin", true, i);
    ++i;
    _implicit_pages.push_back(page);
    _pages_sorted = false;

    std::istringstream in(blobinfo->prc_data);
    page->read_prc(in);
  }

  // Now we have a list of filenames in order from most important to least
  // important.  Walk through the list in reverse order to load their
  // contents, because we want the first file in the list (the most important)
  // to be on the top of the stack.
  ConfigFiles::reverse_iterator ci;
  for (ci = config_files.rbegin(); ci != config_files.rend(); ++ci) {
    const ConfigFile &file = (*ci);
    Filename filename = file._filename;

    if ((file._file_flags & FF_execute) != 0 &&
        filename.is_executable()) {

#ifdef __EMSCRIPTEN__
      prc_cat.error()
        << "Executable config files are not supported with Emscripten.\n";
#else
      // Attempt to execute the file as a command.
      string command = filename.to_os_specific();

      string envvar = PRC_EXECUTABLE_ARGS_ENVVAR;
      if (blobinfo != nullptr && blobinfo->prc_executable_args_envvar != nullptr) {
        envvar = blobinfo->prc_executable_args_envvar;
      }
      if (!envvar.empty()) {
        string args = ExecutionEnvironment::get_environment_variable(envvar);
        if (!args.empty()) {
          command += " ";
          command += args;
        }
      }
      IPipeStream ifs(command);

      ConfigPage *page = new ConfigPage(filename, true, i);
      ++i;
      _implicit_pages.push_back(page);
      _pages_sorted = false;

      page->read_prc(ifs);
#endif  // __EMSCRIPTEN__

    } else if ((file._file_flags & FF_decrypt) != 0) {
      // Read and decrypt the file.
      filename.set_binary();

      pifstream in;
      if (!filename.open_read(in)) {
        prc_cat.error()
          << "Unable to read " << filename << "\n";
      } else {
        ConfigPage *page = new ConfigPage(filename, true, i);
        ++i;
        _implicit_pages.push_back(page);
        _pages_sorted = false;

        if (blobinfo != nullptr && blobinfo->prc_encryption_key != nullptr) {
          page->read_encrypted_prc(in, blobinfo->prc_encryption_key);
        } else {
          page->read_encrypted_prc(in, PRC_ENCRYPTION_KEY);
        }
      }

    } else if ((file._file_flags & FF_read) != 0) {
      // Just read the file.
      filename.set_text();

      pifstream in;
      if (!filename.open_read(in)) {
        prc_cat.error()
          << "Unable to read " << filename << "\n";
      } else {
        ConfigPage *page = new ConfigPage(filename, true, i);
        ++i;
        _implicit_pages.push_back(page);
        _pages_sorted = false;

        page->read_prc(in);
      }
    }
  }
#endif  // ANDROID

  if (!_loaded_implicit) {
    config_initialized();
    _loaded_implicit = true;
  }

  _currently_loading = false;
  invalidate_cache();

#ifdef USE_PANDAFILESTREAM
  // Update this very low-level config variable here, for lack of any better
  // place.
  ConfigVariableEnum<PandaFileStreamBuf::NewlineMode> newline_mode
    ("newline-mode", PandaFileStreamBuf::NM_native,
     PRC_DESC("Controls how newlines are written by Panda applications writing "
              "to a text file.  The default, \"native\", means to write newlines "
              "appropriate to the current platform.  You may also specify \"binary\", "
              "to avoid molesting the file data, or one of \"msdos\", \"unix\", "
              "or \"mac\"."));
  PandaFileStreamBuf::_newline_mode = newline_mode;
#endif  // USE_PANDAFILESTREAM

#ifdef _WIN32
  // We don't necessarily want an error dialog when we fail to load a .dll
  // file.  But sometimes it is useful for debugging.
  ConfigVariableBool show_dll_error_dialog
    ("show-dll-error-dialog", false,
     PRC_DESC("Set this true to enable the Windows system dialog that pops "
              "up when a DLL fails to load, or false to disable it.  It is "
              "normally false, but it may be useful to set it true to debug "
              "why a DLL is not loading.  (Note that this actually disables "
              "*all* critical error messages, and that it's a global setting "
              "that some other libraries might un-set.)"));
  if (show_dll_error_dialog) {
    SetErrorMode(0);
  } else {
    SetErrorMode(SEM_FAILCRITICALERRORS);
  } 
#endif

#endif // LINK_ALL_STATIC

}

/**
 * Creates and returns a new, empty ConfigPage.  This page will be stacked on
 * top of any pages that were created before; it may shadow variable
 * declarations that are defined in previous pages.
 */
ConfigPage *ConfigPageManager::
make_explicit_page(const string &name) {
  ConfigPage *page = new ConfigPage(name, false, _next_page_seq);
  ++_next_page_seq;
  _explicit_pages.push_back(page);
  _pages_sorted = false;
  invalidate_cache();
  return page;
}

/**
 * Removes a previously-constructed ConfigPage from the set of active pages,
 * and deletes it.  The ConfigPage object is no longer valid after this call.
 * Returns true if the page is successfully deleted, or false if it was
 * unknown (which should never happen if the page was legitimately
 * constructed).
 */
bool ConfigPageManager::
delete_explicit_page(ConfigPage *page) {
  Pages::iterator pi;
  for (pi = _explicit_pages.begin(); pi != _explicit_pages.end(); ++pi) {
    if ((*pi) == page) {
      _explicit_pages.erase(pi);
      delete page;
      invalidate_cache();
      return true;
    }
  }
  return false;
}

/**
 *
 */
void ConfigPageManager::
output(std::ostream &out) const {
  out << "ConfigPageManager, "
      << _explicit_pages.size() + _implicit_pages.size()
      << " pages.";
}

/**
 *
 */
void ConfigPageManager::
write(std::ostream &out) const {
  check_sort_pages();
  out << _explicit_pages.size() << " explicit pages:\n";

  Pages::const_iterator pi;
  for (pi = _explicit_pages.begin(); pi != _explicit_pages.end(); ++pi) {
    const ConfigPage *page = (*pi);
    out << "  " << page->get_name();
    if (page->get_trust_level() > 0) {
      out << "  (signed " << page->get_trust_level() << ": ";
      page->output_brief_signature(out);
      out << ")\n";
    } else if (!page->get_signature().empty()) {
      out << "  (invalid signature: ";
      page->output_brief_signature(out);
      out << ")\n";
    } else {
      out << "\n";
    }
  }

  out << "\n" << _implicit_pages.size() << " implicit pages:\n";
  for (pi = _implicit_pages.begin(); pi != _implicit_pages.end(); ++pi) {
    const ConfigPage *page = (*pi);
    out << "  " << page->get_name();
    if (page->get_trust_level() > 0) {
      out << "  (signed " << page->get_trust_level() << ": ";
      page->output_brief_signature(out);
      out << ")\n";
    } else if (!page->get_signature().empty()) {
      out << "  (invalid signature: ";
      page->output_brief_signature(out);
      out << ")\n";
    } else {
      out << "\n";
    }
  }
}

/**
 *
 */
ConfigPageManager *ConfigPageManager::
get_global_ptr() {
  if (_global_ptr == nullptr) {
    _global_ptr = new ConfigPageManager;
  }
  return _global_ptr;
}

// This class is used in sort_pages, below.
class CompareConfigPages {
public:
  bool operator () (const ConfigPage *a, const ConfigPage *b) const {
    return (*a) < (*b);
  }
};

/**
 * Sorts the list of pages into priority order, so that the page at the front
 * of the list is the one that shadows all following pages.
 */
void ConfigPageManager::
sort_pages() {
  sort(_implicit_pages.begin(), _implicit_pages.end(), CompareConfigPages());
  sort(_explicit_pages.begin(), _explicit_pages.end(), CompareConfigPages());

  _pages_sorted = true;
}

/**
 * Checks for the prefix "<auto>" in the value of the $PRC_DIR environment
 * variable (or in the compiled-in DEFAULT_PRC_DIR value).  If it is found,
 * then the actual directory is determined by searching upward from the
 * executable's starting directory, or from the current working directory,
 * until at least one .prc file is found.
 *
 * Returns true if the prc_dir has been filled with a valid directory name,
 * false if no good directory name was found.
 */
bool ConfigPageManager::
scan_auto_prc_dir(Filename &prc_dir) const {
  string prc_dir_string = prc_dir;
  if (prc_dir_string.substr(0, 6) == "<auto>") {
    Filename suffix = prc_dir_string.substr(6);

    // Start at the dtool directory.
    Filename dtool = ExecutionEnvironment::get_dtool_name();
    Filename dir = dtool.get_dirname();

    if (scan_up_from(prc_dir, dir, suffix)) {
      return true;
    }

    // Try the program's directory.
    dir = ExecutionEnvironment::get_environment_variable("MAIN_DIR");
    if (scan_up_from(prc_dir, dir, suffix)) {
      return true;
    }

    // Didn't find it; too bad.
    std::cerr << "Warning: unable to auto-locate config files in directory named by \""
         << prc_dir << "\".\n";
    return false;
  }

  // The filename did not begin with "<auto>", so it stands unchanged.
  return true;
}

/**
 * Used to implement scan_auto_prc_dir(), above, this scans upward from the
 * indicated directory name until a directory is found that includes at least
 * one .prc file, or the root directory is reached.
 *
 * If a match is found, puts it result and returns true; otherwise, returns
 * false.
 */
bool ConfigPageManager::
scan_up_from(Filename &result, const Filename &dir,
             const Filename &suffix) const {
  Filename consider(dir, suffix);

  vector_string files;
  if (consider.is_directory()) {
    if (consider.scan_directory(files)) {
      vector_string::const_iterator fi;
      for (fi = files.begin(); fi != files.end(); ++fi) {
        Globs::const_iterator gi;
        for (gi = _prc_patterns.begin();
             gi != _prc_patterns.end();
             ++gi) {
          if ((*gi).matches(*fi)) {
            result = consider;
            return true;
          }
        }

        for (gi = _prc_executable_patterns.begin();
             gi != _prc_executable_patterns.end();
             ++gi) {
          if ((*gi).matches(*fi)) {
            result = consider;
            return true;
          }
        }
      }
    }
  }

  Filename parent = dir.get_dirname();

  if (dir == parent) {
    // Too bad; couldn't find a match.
    return false;
  }

  // Recursively try again on the parent.
  return scan_up_from(result, parent, suffix);
}

/**
 * This is called once, at startup, the first time that the config system has
 * been initialized and is ready to read config variables.  It's intended to
 * be a place to initialize values that are defined at a lower level than the
 * config system itself.
 */
void ConfigPageManager::
config_initialized() {
  Notify::config_initialized();

  // Also set up some other low-level things.
  ConfigVariableEnum<TextEncoder::Encoding> text_encoding
    ("text-encoding", TextEncoder::E_utf8,
     PRC_DESC("Specifies how international characters are represented in strings "
              "of 8-byte characters presented to Panda.  See TextEncoder::set_encoding()."));
  TextEncoder::set_default_encoding(text_encoding);

  ConfigVariableEnum<TextEncoder::Encoding> filesystem_encoding
    ("filesystem-encoding", TextEncoder::E_utf8,
     PRC_DESC("Specifies the default encoding used for wide-character filenames."));
  Filename::set_filesystem_encoding(filesystem_encoding);

  StringDecoder::set_notify_ptr(&Notify::out());
}
