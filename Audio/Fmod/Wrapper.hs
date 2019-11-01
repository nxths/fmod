{-# LANGUAGE ForeignFunctionInterface #-}
module Audio.Fmod.Wrapper
    ( initFmod
    , freeFmod
    , updateFmod
    , loadFmodSound
    , playFmodSound
    , setFmodChannelLoopCount
    , fadeOutFmodChannel
    , stopFmodChannel
    , playFmodMusic
    , fadeOutFmodMusic
    , fadeInFmodMusic
    , stopFmodMusic
    , muteFmodMusic
    , isFmodMusicPlaying
    , fmodMusicPosition
    , setFmodMusicPosition
    , setFmodSoundVolume
    , setFmodMusicVolume
    , rampMusicToNormalVolume
    , pauseFmodAudio
    , stopFmodAudio
    ) where

import Control.Monad.IO.Class (MonadIO, liftIO)
import Foreign                (free)
import Foreign.C.String       (CString, newCString)
import Foreign.C.Types        (CFloat(CFloat), CInt(CInt), CUInt(CUInt))

import Audio.Fmod.Types
import Audio.Pan
import Audio.Volume

foreign import ccall "initFmod" c_initFmod :: CInt -> IO CInt
foreign import ccall "releaseFmod" c_releaseFmod :: IO CInt
foreign import ccall "updateFmod" c_updateFmod :: IO CInt
foreign import ccall "loadSound" c_loadSound :: CString -> IO CInt
foreign import ccall "playSound" c_playSound :: CInt -> CInt -> CFloat -> IO CInt
foreign import ccall "setLoopCount" c_setLoopCount :: CInt -> CInt -> IO CInt
foreign import ccall "fadeOutChannel" c_fadeOutChannel :: CInt -> IO CInt
foreign import ccall "stopChannel" c_stopChannel :: CInt -> IO CInt
foreign import ccall "playMusic" c_playMusic :: CString -> IO CInt
foreign import ccall "fadeOutMusic" c_fadeOutMusic :: IO CInt
foreign import ccall "fadeInMusic" c_fadeInMusic :: CString -> CFloat -> IO CInt
foreign import ccall "stopMusic" c_stopMusic :: IO CInt
foreign import ccall "muteMusic" c_muteMusic :: CInt -> IO CInt
foreign import ccall "isMusicPlaying" c_isMusicPlaying :: IO CInt
foreign import ccall "musicPosition" c_musicPosition :: IO CUInt
foreign import ccall "setMusicPosition" c_setMusicPosition :: CUInt -> IO CInt
foreign import ccall "setSoundVolume" c_setSoundVolume :: CFloat -> IO CInt
foreign import ccall "setMusicVolume" c_setMusicVolume :: CFloat -> IO CInt
foreign import ccall "rampMusicToNormalVolume" c_rampMusicToNormalVolume :: IO CInt
foreign import ccall "pauseAudio" c_pauseAudio :: CInt -> IO CInt
foreign import ccall "stopAudio" c_stopAudio :: IO CInt

initFmod :: MonadIO m => Int -> m FmodResult
initFmod sampleRateOverride = initFmodResult <$> liftIO (c_initFmod sampleRateOverride')
    where sampleRateOverride' = fromIntegral sampleRateOverride

freeFmod :: MonadIO m => m FmodResult
freeFmod = initFmodResult <$> liftIO c_releaseFmod

updateFmod :: MonadIO m => m FmodResult
updateFmod = initFmodResult <$> liftIO c_updateFmod

loadFmodSound :: MonadIO m => String -> m FmodSoundIndex
loadFmodSound filePath = liftIO $ do
    cFilePath  <- newCString filePath
    soundIndex <- fromIntegral <$> c_loadSound cFilePath
    free cFilePath
    return $ FmodSoundIndex soundIndex

playFmodSound :: MonadIO m => FmodSoundIndex -> FmodPlayMode -> Pan -> m (Maybe FmodChannelIndex)
playFmodSound (FmodSoundIndex soundIndex) playMode pan =
    let
        soundIndex' = fromIntegral soundIndex
        loopCount   = fromIntegral $ case playMode of
            PlayOnce -> 0
            PlayLoop -> -1
        panVal      = realToFrac $ panToFloat pan
    in do
        channelIndex <- fromIntegral <$> liftIO (c_playSound soundIndex' loopCount panVal)
        return $ FmodChannelIndex <$> if
            | channelIndex < 0 -> Nothing
            | otherwise        -> Just channelIndex

setFmodChannelLoopCount :: MonadIO m => FmodChannelIndex -> Int -> m FmodResult
setFmodChannelLoopCount (FmodChannelIndex channelIndex) loopCount =
    initFmodResult <$> liftIO (c_setLoopCount channelIndex' loopCount')
        where
            channelIndex' = fromIntegral channelIndex
            loopCount'    = fromIntegral loopCount

fadeOutFmodChannel :: MonadIO m => FmodChannelIndex -> m FmodResult
fadeOutFmodChannel (FmodChannelIndex channelIndex) = initFmodResult <$> liftIO (c_fadeOutChannel channelIndex')
    where channelIndex' = fromIntegral channelIndex

stopFmodChannel :: MonadIO m => FmodChannelIndex -> m FmodResult
stopFmodChannel (FmodChannelIndex channelIndex) = initFmodResult <$> liftIO (c_stopChannel channelIndex')
    where channelIndex' = fromIntegral channelIndex

playFmodMusic :: MonadIO m => FilePath -> m FmodResult
playFmodMusic filePath = liftIO $ do
    cFilePath <- newCString filePath
    result    <- c_playMusic cFilePath
    free cFilePath
    return $ initFmodResult result

fadeOutFmodMusic :: MonadIO m => m FmodResult
fadeOutFmodMusic = initFmodResult <$> liftIO c_fadeOutMusic

fadeInFmodMusic :: MonadIO m => FilePath -> Volume -> m FmodResult
fadeInFmodMusic filePath volume = do
    cVolume <- realToFrac $ volumeToFloat volume

    liftIO $ do
        cFilePath <- newCString filePath
        result    <- initFmodResult <$> c_fadeInMusic cFilePath cVolume
        free cFilePath
        return result

stopFmodMusic :: MonadIO m => m FmodResult
stopFmodMusic = initFmodResult <$> liftIO c_stopMusic

muteFmodMusic :: MonadIO m => Bool -> m FmodResult
muteFmodMusic mute = initFmodResult <$> liftIO (c_muteMusic mute')
    where mute' = fromIntegral $ if mute then 1 else 0

isFmodMusicPlaying :: MonadIO m => m Bool
isFmodMusicPlaying = do
    isPlaying <- liftIO c_isMusicPlaying
    return $ if isPlaying == 0 then False else True

fmodMusicPosition :: MonadIO m => m FmodResult
fmodMusicPosition = initFmodResult . fromIntegral <$> liftIO c_musicPosition

setFmodMusicPosition :: MonadIO m => Int -> m FmodResult
setFmodMusicPosition pos = initFmodResult <$> liftIO (c_setMusicPosition pos')
    where pos' = fromIntegral pos

setFmodSoundVolume :: MonadIO m => Volume -> m FmodResult
setFmodSoundVolume volume = initFmodResult <$> liftIO (c_setSoundVolume vol)
    where vol = realToFrac $ volumeToFloat volume

setFmodMusicVolume :: MonadIO m => Volume -> m FmodResult
setFmodMusicVolume volume = initFmodResult <$> liftIO (c_setMusicVolume vol)
    where vol = realToFrac $ volumeToFloat volume

rampMusicToNormalVolume :: MonadIO m => m FmodResult
rampMusicToNormalVolume = initFmodResult <$> liftIO c_rampMusicToNormalVolume

pauseFmodAudio :: MonadIO m => Bool -> m FmodResult
pauseFmodAudio paused = initFmodResult <$> liftIO (c_pauseAudio paused')
    where paused' = fromIntegral $ if paused then 1 else 0

stopFmodAudio :: MonadIO m => m FmodResult
stopFmodAudio = initFmodResult <$> liftIO c_stopAudio
