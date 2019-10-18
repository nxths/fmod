module Audio.Fmod.Types
    ( FmodResult(..)
    , initFmodResult
    , FmodSoundIndex(..)
    , FmodChannelIndex(..)
    , FmodPlayMode(..)
    ) where

import Foreign.C.Types (CInt)

newtype FmodResult = FmodResult Int
    deriving Show

initFmodResult :: CInt -> FmodResult
initFmodResult = FmodResult . fromIntegral

newtype FmodSoundIndex   = FmodSoundIndex Int
newtype FmodChannelIndex = FmodChannelIndex Int

data FmodPlayMode
    = PlayOnce
    | PlayLoop
