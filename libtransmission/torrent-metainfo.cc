/*
 * This file Copyright (C) 2007-2014 Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 */

#include <algorithm>
#include <iterator>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>

#include <event2/util.h> // evutil_ascii_strncasecmp

#include "transmission.h"

#include "crypto-utils.h"
#include "error-types.h"
#include "error.h"
#include "torrent-metainfo.h"
#include "tr-assert.h"
#include "utils.h"
#include "variant.h"
#include "web-utils.h"

using namespace std::literals;

#if 0
tr_piece_index_t getBytePiece(tr_torrent_metainfo const& tm, uint64_t byte_offset)
{
    // handle 0-byte files at the end of a torrent
    return byte_offset == tm.total_size ? tm.n_pieces - 1 : byte_offset / tm.piece_size;
}
#endif

#if 0
std::string tr_new_magnet_metainfo::makeFilename(std::string_view dirname, FilenameFormat format, std::string_view suffix) const
{
    // `${dirname}/${name}.${info_hash}${suffix}`
    // `${dirname}/${info_hash}${suffix}`
    return format == FilenameFormat::NameAndParitalHash ?
        tr_strvJoin(dirname, "/"sv, this->name, "."sv, this->infoHashString().substr(0, 16), suffix) :
        tr_strvJoin(dirname, "/"sv, this->infoHashString(), suffix);
}
#endif

/// tr_torrent_metainfo

//// C BINDINGS

/// Lifecycle

#if 0
tr_torrent_metainfo* tr_torrentMetainfoNewFromData(char const* data, size_t data_len, struct tr_error** error)
{
    auto* tm = new tr_torrent_metainfo{};
    if (!tm->parseBenc(std::string_view{ data, data_len }, error))
    {
        delete tm;
        return nullptr;
    }

    return tm;
}
#endif

#if 0
tr_torrent_metainfo* tr_torrentMetainfoNewFromFile(char const* filename, struct tr_error** error)
{
    auto* tm = new tr_torrent_metainfo{};
    if (!tm->parseBencFromFile(filename ? filename : "", nullptr, error))
    {
        delete tm;
        return nullptr;
    }

    return tm;
}
#endif

#if 0
void tr_torrentMetainfoFree(tr_torrent_metainfo* tm)
{
    delete tm;
}
#endif

////  Accessors

#if 0
char* tr_torrentMetainfoMagnet(struct tr_torrent_metainfo const* tm)
{
    return tr_strvDup(tm->magnet());
}
#endif

/// Info

#if 0
tr_torrent_metainfo_info* tr_torrentMetainfoGet(tr_torrent_metainfo const* tm, tr_torrent_metainfo_info* setme)
{
    setme->comment = tm->comment.c_str();
    setme->creator = tm->creator.c_str();
    setme->info_hash = tm->info_hash;
    setme->info_hash_string = std::data(tm->info_hash_chars);
    setme->is_private = tm->is_private;
    setme->n_pieces = tm->n_pieces;
    setme->name = tm->name.c_str();
    setme->source = tm->source.c_str();
    setme->time_created = tm->time_created;
    setme->total_size = tm->total_size;
    return setme;
}
#endif

/// Files

#if 0
size_t tr_torrentMetainfoFileCount(tr_torrent_metainfo const* tm)
{
    return std::size(tm->files);
}
#endif

#if 0
tr_torrent_metainfo_file_info* tr_torrentMetainfoFile(
    tr_torrent_metainfo const* tm,
    size_t n,
    tr_torrent_metainfo_file_info* setme)
{
    auto& file = tm->files[n];
    setme->path = file.path.c_str();
    setme->size = file.size;
    return setme;
}
#endif

/// Trackers

#if 0
size_t tr_torrentMetainfoTrackerCount(tr_torrent_metainfo const* tm)
{
    return std::size(tm->trackers);
}
#endif

#if 0
tr_torrent_metainfo_tracker_info* tr_torrentMetainfoTracker(
    tr_torrent_metainfo const* tm,
    size_t n,
    tr_torrent_metainfo_tracker_info* setme)
{
    auto it = std::begin(tm->trackers);
    std::advance(it, n);
    auto const& tracker = it->second;
    setme->announce_url = tr_quark_get_string(tracker.announce_url);
    setme->scrape_url = tr_quark_get_string(tracker.scrape_url);
    setme->tier = tracker.tier;
    return setme;
}
#endif

#if 0
void tr_metainfoDestruct(tr_info* inf)
{
    for (unsigned int i = 0; i < inf->webseedCount; i++)
    {
        tr_free(inf->webseeds[i]);
    }

    for (tr_file_index_t ff = 0; ff < inf->fileCount; ff++)
    {
        tr_free(inf->files[ff].name);
    }

    tr_free(inf->webseeds);
    tr_free(inf->files);
    tr_free(inf->comment);
    tr_free(inf->creator);
    tr_free(inf->source);
    tr_free(inf->torrent);
    tr_free(inf->originalName);
    tr_free(inf->name);

    for (unsigned int i = 0; i < inf->trackerCount; i++)
    {
        tr_free(inf->trackers[i].announce);
        tr_free(inf->trackers[i].scrape);
    }

    tr_free(inf->trackers);

    memset(inf, '\0', sizeof(tr_info));
}

static std::string getTorrentFilename(tr_session const* session, tr_info const* inf, enum tr_metainfo_basename_format format)
{
    return tr_buildTorrentFilename(tr_getTorrentDir(session), inf, format, ".torrent"sv);
}

void tr_metainfoRemoveSaved(tr_session const* session, tr_torrent_metainfo const& metainfo)
{
    auto filename = getTorrentFilename(session, inf, tr_torrent_metainfo::FilenameFormat::FullHash);
    tr_sys_path_remove(filename.c_str(), nullptr);

    filename = getTorrentFilename(session, inf, tr_torrent_metainfo::FilenameFormat::NameAndParitalHash);
    tr_sys_path_remove(filename.c_str(), nullptr);
}

void tr_metainfoMigrateFile(
    tr_session const* session,
    tr_info const* info,
    enum tr_metainfo_basename_format old_format,
    enum tr_metainfo_basename_format new_format)
{
    auto const old_filename = getTorrentFilename(session, info, old_format);
    auto const new_filename = getTorrentFilename(session, info, new_format);

    if (tr_sys_path_rename(old_filename.c_str(), new_filename.c_str(), nullptr))
    {
        tr_logAddNamedError(
            info->name,
            "Migrated torrent file from \"%s\" to \"%s\"",
            old_filename.c_str(),
            new_filename.c_str());
    }
}
#endif

/***
****
***/

void tr_torrent_metainfo::clear()
{
    *this = tr_torrent_metainfo{};
}

/**
 * @brief Ensure that the URLs for multfile torrents end in a slash.
 *
 * See http://bittorrent.org/beps/bep_0019.html#metadata-extension
 * for background on how the trailing slash is used for "url-list"
 * fields.
 *
 * This function is to workaround some .torrent generators, such as
 * mktorrent and very old versions of utorrent, that don't add the
 * trailing slash for multifile torrents if omitted by the end user.
 */
std::string tr_torrent_metainfo::fixWebseedUrl(tr_torrent_metainfo const& tm, std::string_view url)
{
    url = tr_strvStrip(url);

    if (std::size(tm.files_) > 1 && !std::empty(url) && url.back() != '/')
    {
        return std::string{ url } + '/';
    }

    return std::string{ url };
}

void tr_torrent_metainfo::parseWebseeds(tr_torrent_metainfo& setme, tr_variant* meta)
{
    setme.webseed_urls_.clear();

    auto url = std::string_view{};
    tr_variant* urls = nullptr;
    if (tr_variantDictFindList(meta, TR_KEY_url_list, &urls))
    {
        size_t const n = tr_variantListSize(urls);
        setme.webseed_urls_.reserve(n);
        for (size_t i = 0; i < n; ++i)
        {
            if (tr_variantGetStrView(tr_variantListChild(urls, i), &url) && tr_urlIsValid(url))
            {
                setme.webseed_urls_.push_back(fixWebseedUrl(setme, url));
            }
        }
    }
    else if (tr_variantDictFindStrView(meta, TR_KEY_url_list, &url) && tr_urlIsValid(url)) // handle single items in webseeds
    {
        setme.webseed_urls_.push_back(fixWebseedUrl(setme, url));
    }
}

static bool appendSanitizedComponent(std::string& out, std::string_view in, bool* setme_is_adjusted)
{
    auto const original_out_len = std::size(out);
    auto const original_in = in;
    *setme_is_adjusted = false;

    // remove leading spaces
    auto constexpr leading_test = [](auto ch)
    {
        return isspace(ch);
    };
    auto const it = std::find_if_not(std::begin(in), std::end(in), leading_test);
    in.remove_prefix(std::distance(std::begin(in), it));

    // remove trailing spaces and '.'
    auto constexpr trailing_test = [](auto ch)
    {
        return isspace(ch) || ch == '.';
    };
    auto const rit = std::find_if_not(std::rbegin(in), std::rend(in), trailing_test);
    in.remove_suffix(std::distance(std::rbegin(in), rit));

    // munge banned characters
    // https://docs.microsoft.com/en-us/windows/desktop/FileIO/naming-a-file
    auto constexpr ensure_legal_char = [](auto ch)
    {
        auto constexpr Banned = std::string_view{ "<>:\"/\\|?*" };
        auto const banned = Banned.find(ch) != Banned.npos || (unsigned char)ch < 0x20;
        return banned ? '_' : ch;
    };
    auto const old_out_len = std::size(out);
    std::transform(std::begin(in), std::end(in), std::back_inserter(out), ensure_legal_char);

    // munge banned filenames
    // https://docs.microsoft.com/en-us/windows/desktop/FileIO/naming-a-file
    auto constexpr ReservedNames = std::array<std::string_view, 22>{
        "CON"sv,  "PRN"sv,  "AUX"sv,  "NUL"sv,  "COM1"sv, "COM2"sv, "COM3"sv, "COM4"sv, "COM5"sv, "COM6"sv, "COM7"sv,
        "COM8"sv, "COM9"sv, "LPT1"sv, "LPT2"sv, "LPT3"sv, "LPT4"sv, "LPT5"sv, "LPT6"sv, "LPT7"sv, "LPT8"sv, "LPT9"sv,
    };
    for (auto const& name : ReservedNames)
    {
        size_t const name_len = std::size(name);
        if (evutil_ascii_strncasecmp(out.c_str() + old_out_len, std::data(name), name_len) != 0 ||
            (out[old_out_len + name_len] != '\0' && out[old_out_len + name_len] != '.'))
        {
            continue;
        }

        out.insert(std::begin(out) + old_out_len + name_len, '_');
        break;
    }

    *setme_is_adjusted = original_in != std::string_view{ out.c_str() + original_out_len };
    return std::size(out) > original_out_len;
}

std::string tr_torrent_metainfo::parsePath(std::string_view root, tr_variant* path, std::string& buf)
{
    if (!tr_variantIsList(path))
    {
        return {};
    }

    buf = root;
    for (int i = 0, n = tr_variantListSize(path); i < n; i++)
    {
        auto raw = std::string_view{};
        if (!tr_variantGetStrView(tr_variantListChild(path, i), &raw))
        {
            return {};
        }

        auto is_component_adjusted = bool{};
        auto const pos = std::size(buf);
        if (!appendSanitizedComponent(buf, raw, &is_component_adjusted))
        {
            continue;
        }

        buf.insert(std::begin(buf) + pos, TR_PATH_DELIMITER);
    }

    if (std::size(buf) <= std::size(root))
    {
        return {};
    }

    return tr_strvUtf8Clean(buf);
}

std::string_view tr_torrent_metainfo::parseFiles(tr_torrent_metainfo& setme, tr_variant* info_dict, uint64_t* setme_total_size)
{
    auto is_root_adjusted = bool{ false };
    auto root_name = std::string{};
    auto total_size = uint64_t{ 0 };

    setme.files_.clear();

    if (!appendSanitizedComponent(root_name, setme.name_, &is_root_adjusted))
    {
        return "invalid name"sv;
    }

    // bittorrent 1.0 spec
    // http://bittorrent.org/beps/bep_0003.html
    //
    // "There is also a key length or a key files, but not both or neither.
    //
    // "If length is present then the download represents a single file,
    // otherwise it represents a set of files which go in a directory structure.
    // In the single file case, length maps to the length of the file in bytes.
    auto len = int64_t{};
    tr_variant* files_entry = nullptr;
    if (tr_variantDictFindInt(info_dict, TR_KEY_length, &len))
    {
        total_size = len;
        setme.files_.emplace_back(root_name, len);
    }

    // "For the purposes of the other keys, the multi-file case is treated as
    // only having a single file by concatenating the files in the order they
    // appear in the files list. The files list is the value files maps to,
    // and is a list of dictionaries containing the following keys:
    // length - The length of the file, in bytes.
    // path - A list of UTF-8 encoded strings corresponding to subdirectory
    // names, the last of which is the actual file name (a zero length list
    // is an error case).
    // In the multifile case, the name key is the name of a directory.
    else if (tr_variantDictFindList(info_dict, TR_KEY_files, &files_entry))
    {

        auto buf = std::string{};
        auto const n_files = size_t{ tr_variantListSize(files_entry) };
        for (size_t i = 0; i < n_files; ++i)
        {
            auto* const file_entry = tr_variantListChild(files_entry, i);
            if (!tr_variantIsDict(file_entry))
            {
                return "'files' is not a dictionary";
            }

            if (!tr_variantDictFindInt(file_entry, TR_KEY_length, &len))
            {
                return "length";
            }

            tr_variant* path_variant = nullptr;
            if (!tr_variantDictFindList(file_entry, TR_KEY_path_utf_8, &path_variant) &&
                !tr_variantDictFindList(file_entry, TR_KEY_path, &path_variant))
            {
                return "path";
            }

            auto const path = parsePath(root_name, path_variant, buf);
            if (std::empty(path))
            {
                return "path";
            }

            setme.files_.emplace_back(path, len);
            total_size += len;
        }
    }
    else
    {
        // TODO: add support for 'file tree' BitTorrent 2 torrents / hybrid torrents.
        // Patches welcomed!
        // https://www.bittorrent.org/beps/bep_0052.html#info-dictionary
        return "'info' dict has neither 'files' nor 'length' key";
    }

    *setme_total_size = total_size;
    return {};
}

// https://www.bittorrent.org/beps/bep_0012.html
std::string_view tr_torrent_metainfo::parseAnnounce(tr_torrent_metainfo& setme, tr_variant* meta)
{
    setme.announce_list_.clear();

    auto url = std::string_view{};

    // announce-list
    // example: d['announce-list'] = [ [tracker1], [backup1], [backup2] ]
    tr_variant* tiers = nullptr;
    if (tr_variantDictFindList(meta, TR_KEY_announce_list, &tiers))
    {
        for (size_t i = 0, n_tiers = tr_variantListSize(tiers); i < n_tiers; ++i)
        {
            tr_variant* tier_list = tr_variantListChild(tiers, i);
            if (tier_list == nullptr)
            {
                continue;
            }

            for (size_t j = 0, jn = tr_variantListSize(tier_list); j < jn; ++j)
            {
                if (!tr_variantGetStrView(tr_variantListChild(tier_list, j), &url))
                {
                    continue;
                }

                setme.announce_list_.add(i, url);
            }
        }
    }

    // single 'announce' url
    if (std::empty(setme.announce_list_) && tr_variantDictFindStrView(meta, TR_KEY_announce, &url))
    {
        setme.announce_list_.add(0, url);
    }

    return {};
}

std::string_view tr_torrent_metainfo::parseImpl(tr_torrent_metainfo& setme, tr_variant* meta, std::string_view benc)
{
    int64_t i = 0;
    auto sv = std::string_view{};

    // info_hash: urlencoded 20-byte SHA1 hash of the value of the info key
    // from the Metainfo file. Note that the value will be a bencoded
    // dictionary, given the definition of the info key above.
    tr_variant* info_dict = nullptr;
    if (tr_variantDictFindDict(meta, TR_KEY_info, &info_dict))
    {
        // Calculate the hash of the `info` dict.
        // This is the torrent's unique ID and is central to everything.
        size_t blen = 0;
        char* const bstr = tr_variantToStr(info_dict, TR_VARIANT_FMT_BENC, &blen);
        auto const hash = tr_sha1(std::string_view{ bstr, blen });
        if (!hash)
        {
            tr_free(bstr);
            return "bad info_dict checksum";
        }
        setme.info_hash_ = *hash;
        setme.info_hash_str_ = tr_sha1_to_string(setme.info_hash_);

        // Remember the offset and length of the bencoded info dict.
        // This is important when providing metainfo to magnet peers
        // (see http://bittorrent.org/beps/bep_0009.html for details).
        //
        // Calculating this later from scratch is kind of expensive,
        // so do it here since we've already got the bencoded info dict.
        auto const it = std::search(std::begin(benc), std::end(benc), bstr, bstr + blen);
        setme.info_dict_offset_ = std::distance(std::begin(benc), it);
        setme.info_dict_size_ = blen;

        // In addition, remember the offset of the pieces dictionary entry.
        // This will be useful when we load piece checksums on demand.
        auto constexpr Key = "6:pieces"sv;
        auto constexpr* BKey = std::data(Key);
        auto const pit = std::search(bstr, bstr + blen, BKey, BKey + std::size(Key));
        setme.pieces_offset_ = setme.info_dict_offset_ + (pit - bstr) + std::size(Key);

        tr_free(bstr);
    }
    else
    {
        return "missing 'info' dictionary";
    }

    // name
    if (tr_variantDictFindStrView(info_dict, TR_KEY_name_utf_8, &sv) || tr_variantDictFindStrView(info_dict, TR_KEY_name, &sv))
    {
        setme.setName(tr_strvUtf8Clean(sv));
    }
    else
    {
        return "'info' dictionary has neither 'name.utf-8' nor 'name'";
    }

    // comment (optional)
    if (tr_variantDictFindStrView(meta, TR_KEY_comment_utf_8, &sv) || tr_variantDictFindStrView(meta, TR_KEY_comment, &sv))
    {
        setme.comment_ = tr_strvUtf8Clean(sv);
    }
    else
    {
        setme.comment_.clear();
    }

    // created by (optional)
    if (tr_variantDictFindStrView(meta, TR_KEY_created_by_utf_8, &sv) ||
        tr_variantDictFindStrView(meta, TR_KEY_created_by, &sv))
    {
        setme.creator_ = tr_strvUtf8Clean(sv);
    }
    else
    {
        setme.creator_.clear();
    }

    // creation date (optional)
    if (tr_variantDictFindInt(meta, TR_KEY_creation_date, &i))
    {
        setme.date_created_ = i;
    }
    else
    {
        setme.date_created_ = 0;
    }

    // private (optional)
    if (tr_variantDictFindInt(info_dict, TR_KEY_private, &i) || tr_variantDictFindInt(meta, TR_KEY_private, &i))
    {
        setme.is_private_ = i != 0;
    }
    else
    {
        setme.is_private_ = false;
    }

    // source (optional)
    if (tr_variantDictFindStrView(info_dict, TR_KEY_source, &sv) || tr_variantDictFindStrView(meta, TR_KEY_source, &sv))
    {
        setme.source_ = tr_strvUtf8Clean(sv);
    }
    else
    {
        setme.source_.clear();
    }

    // piece length
    auto piece_size = uint64_t{};
    if (tr_variantDictFindInt(info_dict, TR_KEY_piece_length, &i) && (i > 0))
    {
        piece_size = i;
    }
    else
    {
        return "'info' dict 'piece length' is missing or has an invalid value";
    }

    // pieces
    if (tr_variantDictFindStrView(info_dict, TR_KEY_pieces, &sv) && (std::size(sv) % sizeof(tr_sha1_digest_t) == 0))
    {
        auto const n = std::size(sv) / sizeof(tr_sha1_digest_t);
        setme.pieces_.resize(n);
        std::copy_n(std::data(sv), std::size(sv), reinterpret_cast<char*>(std::data(setme.pieces_)));
    }
    else
    {
        return "'info' dict 'pieces' is missing or has an invalid value";
    }

    // files
    auto total_size = uint64_t{ 0 };
    auto const errstr = parseFiles(setme, info_dict, &total_size);
    if (!std::empty(errstr))
    {
        return errstr;
    }

    if (std::empty(setme.files_) || total_size == 0)
    {
        return "no files found"sv;
    }

    // do the size and piece size match up?
    setme.block_info_.initSizes(total_size, piece_size);
    if (setme.block_info_.n_pieces != std::size(setme.pieces_))
    {
        return "piece count and file sizes do not match";
    }

    parseAnnounce(setme, meta);
    parseWebseeds(setme, meta);

    return {};
}

bool tr_torrent_metainfo::parseBenc(std::string_view benc, tr_error** error)
{
    auto top = tr_variant{};
    if (!tr_variantFromBuf(&top, TR_VARIANT_PARSE_BENC | TR_VARIANT_PARSE_INPLACE, benc, nullptr, error))
    {
        return false;
    }

    auto const errmsg = parseImpl(*this, &top, benc);
    tr_variantFree(&top);
    if (!std::empty(errmsg))
    {
        tr_error_set(error, TR_ERROR_EINVAL, "Error parsing metainfo: %" TR_PRIsv, TR_PRIsv_ARG(errmsg));
        return false;
    }

    return true;
}

bool tr_torrent_metainfo::parseTorrentFile(std::string_view filename, std::vector<char>* contents, tr_error** error)
{
    auto local_contents = std::vector<char>{};

    if (contents == nullptr)
    {
        contents = &local_contents;
    }

    auto const sz_filename = std::string{ filename };
    return tr_loadFile(*contents, sz_filename.c_str(), error) &&
        parseBenc({ std::data(*contents), std::size(*contents) }, error);
}

tr_sha1_digest_t const& tr_torrent_metainfo::pieceHash(tr_piece_index_t piece) const
{
    return this->pieces_[piece];
}