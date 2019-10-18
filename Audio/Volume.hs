module Audio.Volume
    ( Volume
    , initVolume
    , volumeToInt
    , volumeToFloat
    ) where

import Data.Aeson.Types (FromJSON, Parser, Value(Number), parseJSON, typeMismatch)
import Data.Scientific  (toRealFloat)

minValue = 0   :: Int
maxValue = 100 :: Int

newtype Volume = Volume Int
    deriving Show

instance FromJSON Volume where
    parseJSON :: Value -> Parser Volume
    parseJSON (Number v) =
        let v' = round $ toRealFloat v
        in return $ Volume v'
    parseJSON value      = typeMismatch "Volume" value

initVolume :: Int -> Volume
initVolume val
    | val < minValue = Volume minValue
    | val > maxValue = Volume maxValue
    | otherwise      = Volume val

volumeToInt :: Volume -> Int
volumeToInt (Volume val) = val

volumeToFloat :: Volume -> Float
volumeToFloat (Volume val) = realToFrac val / realToFrac maxValue
