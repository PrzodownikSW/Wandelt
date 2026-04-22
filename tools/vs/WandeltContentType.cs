using Microsoft.VisualStudio.Utilities;
using System.ComponentModel.Composition;

namespace Wandelt
{
    internal static class WandeltContentType
    {
        [Export]
        [Name("wdt")]
        [BaseDefinition("code")]
        internal static ContentTypeDefinition WandeltContentTypeDefinition;

        [Export]
        [FileExtension(".wdt")]
        [ContentType("wdt")]
        internal static FileExtensionToContentTypeDefinition WandeltFileExtensionDefinition;
    }
}
