This sample dumps media file's container, media stream and media type 
attributes as well as metadata and DRM properties.

The sample uses MF media source to read presentation descriptor attributes, 
stream attributes and media type attributes on each stream. Also it dumps 
MF and shell metadata.
For protected media files DRM properties are dumped using FSDK.


Usage:
------

Command line options:

    -?, /?, -h, /h

        display usage information

    -f filename, /f filename
        set the name of the file to dump properties

        
Examples:
---------

Dump WMV file properties:

    MFMediaPropDump.exe -f input.wmv
    
Dump MPEG-4 file properties:

    MFMediaPropDump.exe -f input.mp4
    
Dump MP3 file properties:

    MFMediaPropDump.exe -f input.mp3
    
Dump AVI file properties:

    MFMediaPropDump.exe -f input.avi

    
API demonstrated:
-----------------

    MFStartup

    MFShutdown

    MFCreateSourceResolver

    IMFSourceResolver
        ::CreateObjectFromURL

    IMFMediaSource
        ::CreatePresentationDescriptor

    IMFPresentationDescriptor
        ::GetStreamDescriptorCount
        ::GetStreamDescriptorByIndex
        ::SelectStream

    IMFStreamDescriptor
        ::GetMediaTypeHandler

    IMFMediaTypeHandler
        ::GetMediaTypeCount
        ::GetMediaTypeByIndex

    IMFMediaType

    IMFAttributes
        ::GetCount
        ::GetItemByIndex

    IMFMetadata
        ::GetAllPropertyNames
        ::GetProperty

    IPropertyStore
        ::GetCount
        ::GetAt

    IMFGetService
        ::GetService

    WMCreateEditor

    IWMMetadataEditor2
        ::OpenEx

    IWMMetadataEditor
        ::Close

    IWMDRMEditor
        ::GetDRMProperty


Requirements:
-------------

OS: Windows 7


Known limitations:
------------------

*   For proper dumping of DRM properties in a file its license should be 
    acquired first. This can be done by a simple playback in Windows Media 
    Player which will trigger the license acquisition (either silent or 
    non-silent which prompts the user to acquire the license). Once the 
    license is acquired user can see DRM properties in the file with 
    MFMediaPropDump.

*   This sample can dump properties of media files with any format supported 
    by Media Foundation. It will fail to dump other files' properties.

*   For ASF files with multi-language Media Foundation metadata the tool 
    currently dumps only the default language metadata. 
