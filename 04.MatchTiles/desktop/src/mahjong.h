
/*
This file is part of OGS Mahjong components:
  https://github.com/OGStudio/ogs-mahjong-components

Copyright (C) 2018 Opensource Game Studio

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef OGS_MAHJONG_COMPONENTS_MAHJONG_H
#define OGS_MAHJONG_COMPONENTS_MAHJONG_H

// MC_MAHJONG_LOG Start
#include "log.h"
#include "format.h"
#define MC_MAHJONG_LOG_PREFIX "mahjong %s"
#define MC_MAHJONG_LOG(...) \
    log::logprintf( \
        MC_MAHJONG_LOG_PREFIX, \
        format::printfString(__VA_ARGS__).c_str() \
    )
// MC_MAHJONG_LOG End

namespace mc
{
namespace mahjong
{

// TilePosition Start
struct TilePosition
{
    TilePosition() :
        field(0),
        row(0),
        column(0)
    { }

    TilePosition(int field, int row, int column) :
        field(field),
        row(row),
        column(column)
    { }

    int field;
    int row;
    int column;
};
// TilePosition End
// Tile Start
struct Tile
{
    Tile() : matchId(0) { }

    // NOTE Try not to use id since it's an external representation.
    //int id;
    int matchId;
    TilePosition position;
};
// Tile End

// Layout Start
//! Layout representation.
struct Layout
{
    typedef std::vector<TilePosition> Positions;

    int width;
    int height;
    int depth;
    Positions positions;
};
// Layout End
// KMahjonggLayout Start
//! KMahjongg layout representation to use only during parsing.
struct KMahjonggLayout : Layout
{
    std::string version;
};
// KMahjonggLayout End

// intToTilePosition Start
TilePosition intToTilePosition(int value)
{
    int field = value / 1000000;
    int row = (value - field * 1000000) / 1000;
    int column = value - field * 1000000 - row * 1000;
    return TilePosition(field, row, column);
}
// intToTilePosition End
// tilePositionToInt Start
int tilePositionToInt(const TilePosition &position)
{
    return position.field * 1000000 + position.row * 1000 + position.column;
}
// tilePositionToInt End

// kmahjonggLayoutFieldsToPositions Start
typedef std::vector<std::string> Field;
typedef std::vector<Field> Fields;

KMahjonggLayout::Positions kmahjonggLayoutFieldsToPositions(
    const Fields &fields,
    int width,
    int height
) {
    KMahjonggLayout::Positions positions;
    for (int fieldId = 0; fieldId < fields.size(); ++fieldId)
    {
        auto field = fields[fieldId];
        for (int row = 0; row < height - 1; ++row)
        {
            for (int column = 0; column < width - 1; ++column)
            {
                // Detect tile.
                if (
                    (field[row][column] == '1') &&
                    (field[row][column + 1] == '2') &&
                    (field[row + 1][column] == '4') &&
                    (field[row + 1][column + 1] == '3')
                ) {
                    positions.push_back({fieldId, row, column});
                }
            }
        }
    }
    return positions;
}
// kmahjonggLayoutFieldsToPositions End
// linesToKMahjonggLayout Start
bool linesToKMahjonggLayout(
    const std::vector<std::string> &lines,
    KMahjonggLayout &layout
) {
    KMahjonggLayout layoutDraft;
    // Setup defaults.
    // KMahjongg layouts of version 1.0 have 32x16 field size.
    // So we simply set these values as defaults.
    layoutDraft.width = 32;
    layoutDraft.height = 16;
    // Depth is counted for version 1.0. So set it to zero.
    layoutDraft.depth = 0;

    // Prefixes.
    const std::string PREFIX_COMMENT = "#";
    const std::string PREFIX_VERSION = "kmahjongg-layout-v";
    const std::string PREFIX_DEPTH = "d";
    const std::string PREFIX_WIDTH = "w";
    const std::string PREFIX_HEIGHT = "h";

    Fields fields;
    Field field;
    // Count field lines to detect end-of-field.
    int fieldLineId = 0;

    for (auto ln : lines)
    {
        // Ignore comment.
        if (format::stringStartsWith(ln, PREFIX_COMMENT))
        {
            continue;
        }
        // Remove spaces.
        auto sln = format::trimmedString(ln);
        // Version.
        if (format::stringStartsWith(sln, PREFIX_VERSION))
        {
            layoutDraft.version = sln.substr(PREFIX_VERSION.length());
            // Explicitely support expected versions.
            if (
                layoutDraft.version != "1.0" &&
                layoutDraft.version != "1.1"
            ) {
                return false;
            }
        }
        // Depth.
        else if (format::stringStartsWith(sln, PREFIX_DEPTH))
        {
            auto depth = sln.substr(PREFIX_DEPTH.length());
            layoutDraft.depth = atoi(depth.c_str());
        }
        // Width.
        else if (format::stringStartsWith(sln, PREFIX_WIDTH))
        {
            auto width = sln.substr(PREFIX_WIDTH.length());
            layoutDraft.width = atoi(width.c_str());
        }
        // Height.
        else if (format::stringStartsWith(sln, PREFIX_HEIGHT))
        {
            auto height = sln.substr(PREFIX_HEIGHT.length());
            layoutDraft.height = atoi(height.c_str());
        }
        else
        {
            // Add line to current field.
            field.push_back(sln);
            ++fieldLineId;
            // Once we have collected enough lines, add current
            // field to the list of fields.
            if (fieldLineId >= layoutDraft.height)
            {
                fields.push_back(field);
                // Reset field buffer for the next field lines.
                field.clear();
                fieldLineId = 0;
            }
        }
    }
    // Make sure depth is equal to the number of fields
    // if depth was specified in layout.
    if (layoutDraft.depth)
    {
        if (fields.size() != layoutDraft.depth)
        {
            MC_MAHJONG_LOG(
                "ERROR Specified layout depth (%d) is not equal to actual one (%d)",
                layoutDraft.depth,
                fields.size()
            );
            return false;
        }
    }
    // Provide layout depth if it was counted.
    else
    {
        layoutDraft.depth = fields.size();
    }
    // Positions.
    layoutDraft.positions =
        kmahjonggLayoutFieldsToPositions(
            fields,
            layoutDraft.width,
            layoutDraft.height
        );

    // Provide cooked layout.
    layout = layoutDraft;
    return true;
}
// linesToKMahjonggLayout End
// parseLayout Start
bool parseLayout(std::istream &in, Layout &layout)
{
    // Collect lines.
    std::vector<std::string> lines;
    std::string ln;
    while (std::getline(in, ln))
    {
        lines.push_back(ln);
    }

    // Parse them.
    KMahjonggLayout kmlayout;
    auto success = linesToKMahjonggLayout(lines, kmlayout);

    // Provide layout if parsing was successful.
    if (success)
    {
        layout = kmlayout;
    }
    
    return success;
}
// parseLayout End

// Solitaire Start
class Solitaire
{
    public:
        Solitaire() { }

        void setTiles(const std::vector<Tile> &tiles)
        {
            int id = 0;
            for (auto tile : tiles)
            {
                // Keep tiles in `id->tile` map for lookup by id.
                this->idTiles[id++] = tile;

                // Keep tiles in `position->tile` map for lookup by position.
                int position = tilePositionToInt(tile.position);
                this->positionTiles[position] = tile;
            }
        }

        bool isTileSelectable(int id)
        {
            // Make sure tile is valid.
            auto tile = this->tile(id);
            if (!tile)
            {
                return false;
            }

            // See if tile is blocked at the left and the right.
            bool left = this->tileHasNeighbours(tile->position, 0, -2);
            bool right = this->tileHasNeighbours(tile->position, 0, 2);
            if (left && right)
            {
                return false;
            }

            // See if tile is blocked at the top.
            for (int columnOffset = -1; columnOffset <= 1; ++columnOffset)
            {
                if (this->tileHasNeighbours(tile->position, 1, columnOffset))
                {
                    return false;
                }
            }

            // Tile is not blocked.
            return true;
        }

        bool tilesMatch(int id1, int id2)
        {
            // Make sure tiles are valid.
            Tile *tile1 = this->tile(id1);
            Tile *tile2 = this->tile(id2);
            if (!tile1 || !tile2)
            {
                return false;
            }

            // Match tiles.
            return (tile1->matchId == tile2->matchId);
        }

        void removeTiles(int id1, int id2)
        {
            this->removeTile(id1);
            this->removeTile(id2);
        }

    private:
        // Id -> Tile.
        std::map<int, Tile> idTiles;
        // Position -> Tile.
        std::map<int, Tile> positionTiles;

        void removeTile(int id)
        {
            // Make sure tile exists.
            auto it = this->idTiles.find(id);
            if (it == this->idTiles.end())
            {
                return;
            }
            auto tile = it->second;

            // Erase tile from idTiles.
            this->idTiles.erase(it);

            // Erase tile from positionTiles.
            auto position = tilePositionToInt(tile.position);
            auto positionIt = this->positionTiles.find(position);
            this->positionTiles.erase(positionIt);
        }

        Tile *tile(int id)
        {
            auto it = this->idTiles.find(id);
            if (it != this->idTiles.end())
            {
                return &it->second;
            }
            return 0;
        }

        bool tileHasNeighbours(
            const TilePosition &tilePosition,
            int fieldOffset,
            int columnOffset
        ) {
            for (int rowOffset = -1; rowOffset <= 1; ++rowOffset)
            {
                // Construct neighbour position.
                auto neighbourPosition =
                    TilePosition(
                        tilePosition.field + fieldOffset,
                        tilePosition.row + rowOffset,
                        tilePosition.column + columnOffset
                    );
                auto position = tilePositionToInt(neighbourPosition);
                // See if it's occupied by the neighbour.
                auto it = this->positionTiles.find(position);
                if (it != this->positionTiles.end())
                {
                    return true;
                }
            }

            // Found no neighbours.
            return false;
        }
};
// Solitaire End

} // namespace mahjong
} // namespace mc

#endif // OGS_MAHJONG_COMPONENTS_MAHJONG_H
