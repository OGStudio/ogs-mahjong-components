FEATURE mahjong.h/Impl
int tilePositionToInt(const TilePosition &position)
{
    return position.fieldId * 1000000 + position.row * 1000 + position.column;
}
