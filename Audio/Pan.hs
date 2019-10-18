module Audio.Pan
    ( Pan
    , initPan
    , panToFloat
    ) where

minValue = -1.0 :: Float
maxValue = 1.0  :: Float

newtype Pan = Pan Float
    deriving Show

initPan :: Float -> Pan
initPan val
    | val < minValue = Pan minValue
    | val > maxValue = Pan maxValue
    | otherwise      = Pan val

panToFloat :: Pan -> Float
panToFloat (Pan val) = realToFrac val
