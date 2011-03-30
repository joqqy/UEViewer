#ifndef __UNMATERIAL_H__
#define __UNMATERIAL_H__

// Ploygon flags (used in UE1 only ?)
#define PF_Masked			0x00000002
#define PF_Translucent		0x00000004
#define PF_Modulated		0x00000040
#define PF_TwoSided			0x00000100


/*
UE2 MATERIALS TREE:
~~~~~~~~~~~~~~~~~~~
-	Material
-		Combiner
-		Modifier
			ColorModifier
-			FinalBlend
			MaterialSequence
			MaterialSwitch
			OpacityModifier
-			TexModifier
				TexCoordSource
-				TexEnvMap
				TexMatrix
-				TexOscillator
					TexOscillatorTriggered
-				TexPanner
					TexPannerTriggered
-				TexRotator
-				TexScaler
				VariableTexPanner
-		RenderedMaterial
-			BitmapMaterial
				ScriptedTexture
				ShadowBitmapMaterial
-				Texture
					Cubemap
-			ConstantMaterial
-				ConstantColor
				FadeColor
			ParticleMaterial
			ProjectorMaterial
-			Shader
			TerrainMaterial
			VertexColor
#if UC2
			PixelShaderMaterial
				PSEmissiveShader
				PSSkinShader
					PSEmissiveSkinShader
#endif

UE3 MATERIALS TREE:
~~~~~~~~~~~~~~~~~~~
	Surface (empty)
		Texture
			Texture2D
				ShadowMapTexture2D
				TerrainWeightMapTexture
				TextureFlipBook
			Texture2DComposite
			Texture3D
			TextureCube
			TextureMovie
			TextureRenderTarget
				TextureRenderTarget2D
				TextureRenderTargetCube
		MaterialInterface (empty)
			Material
				MaterialExpression
					MaterialExpressionTextureSample
					...
			MaterialInstance
				MaterialInstanceConstant
				MaterialInstanceTimeVarying
*/


/*-----------------------------------------------------------------------------
	Internal base material class
-----------------------------------------------------------------------------*/

enum ETextureCannel
{
	TC_NONE = 0,
	TC_R,
	TC_G,
	TC_B,
	TC_A,
	TC_MA					// 1-Alpha
};


class UUnrealMaterial;

struct CMaterialParams
{
	CMaterialParams()
	{
		memset(this, 0, sizeof(*this));
		EmissiveColor.Set(0.5f, 0.5f, 1.0f, 1);		// light-blue color
	}
	bool IsNull() const
	{
#define C(x) ((size_t)x)
		return (C(Diffuse) | C(Normal) | C(Specular) | C(SpecPower) | C(Opacity) | C(Emissive) | C(Cube)) == 0;
#undef C
	}

	// textures
	UUnrealMaterial *Diffuse;
	UUnrealMaterial *Normal;
	UUnrealMaterial *Specular;
	UUnrealMaterial *SpecPower;
	UUnrealMaterial *Opacity;
	UUnrealMaterial *Emissive;
	UUnrealMaterial *Cube;
	UUnrealMaterial	*Mask;					// multiple mask textures baked into a single one
	// channels
	ETextureCannel	EmissiveChannel;
	ETextureCannel	SpecularMaskChannel;
	ETextureCannel	SpecularPowerChannel;
	ETextureCannel	CubemapMaskChannel;
	// colors
	FLinearColor	EmissiveColor;
	// mobile
	bool			bUseMobileSpecular;
	float			MobileSpecularPower;
	int				MobileSpecularMask;		// EMobileSpecularMask
	// tweaks
	bool			SpecularFromAlpha;
	bool			OpacityFromAlpha;
};


class UUnrealMaterial : public UObject		// no such class in Unreal Engine, needed as common base for UE1/UE2/UE3
{
	DECLARE_CLASS(UUnrealMaterial, UObject);
public:
	virtual byte *Decompress(int &USize, int &VSize) const
	{
		return NULL;
	}
#if RENDERING
	UUnrealMaterial()
	:	DrawTimestamp(0)
	{}

	void SetMaterial(unsigned PolyFlags = 0);	// main function to use outside

	virtual void Bind()							// non-empty for textures only
	{}
	virtual void SetupGL(unsigned PolyFlags);	// PolyFlags used for UE1 only
	virtual void Release();
	virtual void GetParams(CMaterialParams &Params) const
	{}
	virtual bool IsTranslucent() const
	{
		return false;
	}
	virtual bool IsTexture() const
	{
		return false;
	}
	virtual bool IsTextureCube() const
	{
		return false;
	}

protected:
	// rendering implementation fields
	CShader			GLShader;
	int				DrawTimestamp;				// timestamp for object validation
		//?? may be use GLShader.DrawTimestamp only ?
#endif // RENDERING
};


/*-----------------------------------------------------------------------------
	Unreal Engine 1/2 materials
-----------------------------------------------------------------------------*/

#if LINEAGE2

struct FLineageMaterialStageProperty
{
	FString			unk1;
	TArray<FString>	unk2;

	friend FArchive& operator<<(FArchive &Ar, FLineageMaterialStageProperty &P)
	{
		return Ar << P.unk1 << P.unk2;
	}
};

struct FLineageShaderProperty
{
	// possibly, MaterialInfo, TextureTranform, TwoPassRenderState, AlphaRef
	byte			b1[4];
	// possibly, SrcBlend, DestBlend, OverriddenFogColor
	int				i1[3];
	// nested structure
	// possibly, int FC_Color1, FC_Color2 (strange byte order)
	byte			b2[8];
	// possibly, float FC_FadePeriod, FC_FadePhase, FC_ColorFadeType
	int				i2[3];
	// stages
	TArray<FLineageMaterialStageProperty> Stages;

	friend FArchive& operator<<(FArchive &Ar, FLineageShaderProperty &P)
	{
		int i;
		for (i = 0; i < 4; i++) Ar << P.b1[i];
		for (i = 0; i < 3; i++) Ar << P.i1[i];
		for (i = 0; i < 8; i++) Ar << P.b2[i];
		for (i = 0; i < 3; i++) Ar << P.i2[i];
		Ar << P.Stages;
		return Ar;
	}
};

#endif // LINEAGE2

#if BIOSHOCK

enum EMaskChannel
{
	MC_A,
	MC_R,
	MC_G,
	MC_B
};

class UMaterial;

struct FMaskMaterial
{
	DECLARE_STRUCT(FMaskMaterial);
	UMaterial		*Material;
	EMaskChannel	Channel;

	BEGIN_PROP_TABLE
		PROP_OBJ(Material)
		PROP_ENUM(Channel)
	END_PROP_TABLE
};

#endif // BIOSHOCK


class UMaterial : public UUnrealMaterial	// real base is UObject
{
	DECLARE_CLASS(UMaterial, UUnrealMaterial);
public:
	UMaterial		*FallbackMaterial;
	UMaterial		*DefaultMaterial;
	byte			SurfaceType;			// enum ESurfaceType: wood, metal etc
//	int				MaterialType;			// int for UE2, but byte for EXTEEL
#if SPLINTER_CELL
	// Splinter Cell extra fields
	bool			bUseTextureAsHeat;
	UObject			*HeatMaterial;
#endif

	UMaterial()
	:	FallbackMaterial(NULL)
//	,	DefaultMaterial(DefaultTexture)
	{}
	BEGIN_PROP_TABLE
		PROP_OBJ(FallbackMaterial)
		PROP_OBJ(DefaultMaterial)
		PROP_ENUM(SurfaceType)
//		PROP_INT(MaterialType)
		PROP_DROP(MaterialType)
#if SPLINTER_CELL
		PROP_BOOL(bUseTextureAsHeat)
		PROP_OBJ(HeatMaterial)
#endif // SPLINTER_CELL
#if BIOSHOCK
		PROP_DROP(MaterialVisualType)
		PROP_DROP(Subtitle)
		PROP_DROP(AcceptProjectors)
#endif // BIOSHOCK
	END_PROP_TABLE

#if LINEAGE2
	virtual void Serialize(FArchive &Ar)
	{
		guard(UMaterial::Serialize);
		Super::Serialize(Ar);
		if (Ar.Game == GAME_Lineage2)
		{
			guard(SerializeLineage2Material);
			//?? separate to cpp
			if (Ar.ArVer >= 123 && Ar.ArLicenseeVer >= 0x10 && Ar.ArLicenseeVer < 0x25)
			{
				int unk1;
				Ar << unk1;					// simply drop obsolete variable (int Reserved ?)
			}
			if (Ar.ArVer >= 123 && Ar.ArLicenseeVer >= 0x1E && Ar.ArLicenseeVer < 0x25)
			{
				int i;
				// some function
				byte MaterialInfo, TextureTranform, MAX_SAMPLER_NUM, MAX_TEXMAT_NUM, MAX_PASS_NUM, TwoPassRenderState, AlphaRef;
				if (Ar.ArLicenseeVer >= 0x21 && Ar.ArLicenseeVer < 0x24)
					Ar << MaterialInfo;
				Ar << TextureTranform << MAX_SAMPLER_NUM << MAX_TEXMAT_NUM << MAX_PASS_NUM << TwoPassRenderState << AlphaRef;
				int SrcBlend, DestBlend, OverriddenFogColor;
				Ar << SrcBlend << DestBlend << OverriddenFogColor;
				// serialize matTexMatrix[16] (strange code)
				for (i = 0; i < 8; i++)
				{
					char b1, b2;
					Ar << b1;
					if (Ar.ArLicenseeVer < 0x24) Ar << b2;
					for (int j = 0; j < 126; j++)
					{
						// really, 1Kb of floats and ints ...
						char b3;
						Ar << b3;
					}
				}
				// another nested function - serialize FC_* variables
				char c[8];					// union with "int FC_Color1, FC_Color2" (strange byte order)
				Ar << c[2] << c[1] << c[0] << c[3] << c[6] << c[5] << c[4] << c[7];
				int FC_FadePeriod, FC_FadePhase, FC_ColorFadeType;	// really, floats?
				Ar << FC_FadePeriod << FC_FadePhase << FC_ColorFadeType;
				// end of nested function
				for (i = 0; i < 16; i++)
				{
					FString strTex;			// strTex[16]
					Ar << strTex;
				}
				// end of function
				FString ShaderCode;
				Ar << ShaderCode;
			}
			if (Ar.ArVer >= 123 && Ar.ArLicenseeVer >= 0x25)
			{
				// ShaderProperty + ShaderCode
				FLineageShaderProperty ShaderProp;
				FString ShaderCode;
				Ar << ShaderProp << ShaderCode;
			}
			if (Ar.ArVer >= 123 && Ar.ArLicenseeVer >= 0x1F)
			{
				word ver1, ver2;			// 'int MaterialCodeVersion' serialized as 2 words
				Ar << ver1 << ver2;
			}
			unguard;
		}
		unguard;
	}
#endif // LINEAGE2
};


class URenderedMaterial : public UMaterial
{
	DECLARE_CLASS(URenderedMaterial, UMaterial);
	// no properties here
};


class UConstantMaterial : public URenderedMaterial
{
	DECLARE_CLASS(UConstantMaterial, URenderedMaterial);
};


class UConstantColor : public UConstantMaterial
{
	DECLARE_CLASS(UConstantColor, UConstantMaterial);
public:
	FColor			Color;

	BEGIN_PROP_TABLE
		PROP_COLOR(Color)
	END_PROP_TABLE
};


enum ETextureFormat
{
	TEXF_P8,			// used 8-bit palette
	TEXF_RGBA7,
	TEXF_RGB16,			// 16-bit texture
	TEXF_DXT1,
	TEXF_RGB8,
	TEXF_RGBA8,			// 32-bit texture
	TEXF_NODATA,
	TEXF_DXT3,
	TEXF_DXT5,
	TEXF_L8,			// 8-bit grayscale
	TEXF_G16,			// 16-bit grayscale (terrain heightmaps)
	TEXF_RRRGGGBBB,
	// Tribes texture formats
	TEXF_CxV8U8,
	TEXF_DXT5N,			// Note: in Bioshock this value has name 3DC, but really DXT5N is used
	TEXF_3DC,			// names: 3Dc, ATI2, BC5
#if IPHONE
	TEXF_PVRTC2,
	TEXF_PVRTC4,
#endif
};

_ENUM(ETextureFormat)
{
	_E(TEXF_P8),
	_E(TEXF_RGBA7),
	_E(TEXF_RGB16),
	_E(TEXF_DXT1),
	_E(TEXF_RGB8),
	_E(TEXF_RGBA8),
	_E(TEXF_NODATA),
	_E(TEXF_DXT3),
	_E(TEXF_DXT5),
	_E(TEXF_L8),
	_E(TEXF_G16),
	_E(TEXF_RRRGGGBBB),
	// Tribes texture formats
	_E(TEXF_CxV8U8),
	_E(TEXF_DXT5N),
	_E(TEXF_3DC),
#if IPHONE
	_E(TEXF_PVRTC2),
	_E(TEXF_PVRTC4),
#endif
};

enum ETexClampMode
{
	TC_Wrap,
	TC_Clamp
};

_ENUM(ETexClampMode)
{
	_E(TC_Wrap),
	_E(TC_Clamp)
};

class UBitmapMaterial : public URenderedMaterial // abstract
{
	DECLARE_CLASS(UBitmapMaterial, URenderedMaterial);
public:
	ETextureFormat	Format;
	ETexClampMode	UClampMode, VClampMode;
	byte			UBits, VBits;	// texture size log2 (number of bits in size value)
	int				USize, VSize;	// texture size
	int				UClamp, VClamp;	// same as UClampMode/VClampMode ?

	BEGIN_PROP_TABLE
		PROP_ENUM2(Format, ETextureFormat)
		PROP_ENUM2(UClampMode, ETexClampMode)
		PROP_ENUM2(VClampMode, ETexClampMode)
		PROP_BYTE(UBits)
		PROP_BYTE(VBits)
		PROP_INT(USize)
		PROP_INT(VSize)
		PROP_INT(UClamp)
		PROP_INT(VClamp)
#if BIOSHOCK
		PROP_DROP(Type)
#endif
	END_PROP_TABLE

#if RENDERING
	virtual bool IsTexture() const
	{
		return true;
	}
#endif // RENDERING
};


class UPalette : public UObject
{
	DECLARE_CLASS(UPalette, UObject);
public:
	TArray<FColor>	Colors;

	virtual void Serialize(FArchive &Ar)
	{
		guard(UPalette::Serialize);
		Super::Serialize(Ar);
		Ar << Colors;
		assert(Colors.Num() == 256);	// NUM_PAL_COLORS in UT
		// UE1 uses Palette[0] as color {0,0,0,0} when texture uses PF_Masked flag
		// (see UOpenGLRenderDevice::SetTexture())
		// Note: UT2 does not use paletted textures, but HP3 does ...
		Colors[0].A = 0;
#if UNDYING
		if (Ar.Game == GAME_Undying)
		{
			int unk;
			Ar << unk;
		}
#endif // UNDYING
		unguard;
	}
};


enum ELODSet
{
	LODSET_None,
	LODSET_World,
	LODSET_PlayerSkin,
	LODSET_WeaponSkin,
	LODSET_Terrain,
	LODSET_Interface,
	LODSET_RenderMap,
	LODSET_Lightmap,
};


struct FMipmap
{
	TLazyArray<byte>	DataArray;
	int					USize, VSize;
	byte				UBits, VBits;

	friend FArchive& operator<<(FArchive &Ar, FMipmap &M)
	{
#if TRIBES3
		TRIBES_HDR(Ar, 0x1A);
		if (t3_hdrSV == 1)
		{
			TLazyArray<byte> SkipArray;
			Ar << SkipArray;
		}
#endif // TRIBES3
		Ar << M.DataArray << M.USize << M.VSize << M.UBits << M.VBits;
		return Ar;
	}
};


class UTexture : public UBitmapMaterial
{
	DECLARE_CLASS(UTexture, UBitmapMaterial);
public:
	// palette
	UPalette		*Palette;
	// detail texture
	UMaterial		*Detail;
	float			DetailScale;
	// internal info
	FColor			MipZero;
	FColor			MaxColor;
	int				InternalTime[2];
	// texture flags
	bool			bMasked;
	bool			bAlphaTexture;
	bool			bTwoSided;
	bool			bHighColorQuality;
	bool			bHighTextureQuality;
	bool			bRealtime;
	bool			bParametric;
	// level of detail
	ELODSet			LODSet;
	int				NormalLOD;
	int				MinLOD;
	// animation
	UTexture		*AnimNext;
	byte			PrimeCount;
	float			MinFrameRate, MaxFrameRate;
	// mipmaps
	TArray<FMipmap>	Mips;				// native
	ETextureFormat	CompFormat;			// UT1, SplinterCell, Tribes3
	// UE1 fields
	byte			bHasComp;
#if BIOSHOCK
	int64			CachedBulkDataSize;
	bool			HasBeenStripped;
	byte			StrippedNumMips;
	bool			bBaked;
#endif

#if RENDERING
	// rendering implementation fields
	GLuint			TexNum;
#endif

	UTexture()
	:	LODSet(LODSET_World)
	,	MipZero(64, 128, 64, 0)
	,	MaxColor(255, 255, 255, 255)
	,	DetailScale(8.0f)
#if RENDERING
	,	TexNum(0)
#endif
	{}
	virtual byte *Decompress(int &USize, int &VSize) const;
	virtual void Serialize(FArchive &Ar)
	{
		guard(UTexture::Serialize);
		Super::Serialize(Ar);
#if BIOSHOCK
		TRIBES_HDR(Ar, 0x2E);
		if (Ar.Game == GAME_Bioshock && t3_hdrSV >= 1)
			Ar << CachedBulkDataSize;
		if (Ar.Game == GAME_Bioshock && Format == 12)	// remap format; note: Bioshock used 3DC name, but real format is DXT5N
			Format = TEXF_DXT5N;
#endif // BIOSHOCK
#if SWRC
		if (Ar.Game == GAME_RepCommando)
		{
			if (Format == 14) Format = TEXF_CxV8U8;		//?? not verified
		}
#endif // SWRC
		Ar << Mips;
		if (Ar.Engine() == GAME_UE1)
		{
			// UE1
			bMasked = false;			// ignored by UE1, used surface.PolyFlags instead (but UE2 ignores PolyFlags ...)
			if (bHasComp)				// skip compressed mipmaps
			{
				TArray<FMipmap>	CompMips;
				Ar << CompMips;
			}
		}
#if EXTEEL
		if (Ar.Game == GAME_Exteel)
		{
			// note: this property is serialized as UObject's property too
			byte MaterialType;			// enum GFMaterialType
			Ar << MaterialType;
		}
#endif // EXTEEL
		unguard;
	}
	BEGIN_PROP_TABLE
		PROP_OBJ(Palette)
		PROP_OBJ(Detail)
		PROP_FLOAT(DetailScale)
		PROP_COLOR(MipZero)
		PROP_COLOR(MaxColor)
		PROP_INT(InternalTime)
		PROP_BOOL(bMasked)
		PROP_BOOL(bAlphaTexture)
		PROP_BOOL(bTwoSided)
		PROP_BOOL(bHighColorQuality)
		PROP_BOOL(bHighTextureQuality)
		PROP_BOOL(bRealtime)
		PROP_BOOL(bParametric)
		PROP_ENUM(LODSet)
		PROP_INT(NormalLOD)
		PROP_INT(MinLOD)
		PROP_OBJ(AnimNext)
		PROP_BYTE(PrimeCount)
		PROP_FLOAT(MinFrameRate)
		PROP_FLOAT(MaxFrameRate)
		PROP_ENUM(CompFormat)
		PROP_BOOL(bHasComp)
#if BIOSHOCK
		PROP_BOOL(HasBeenStripped)
		PROP_BYTE(StrippedNumMips)
		PROP_BOOL(bBaked)
		PROP_DROP(ResourceCategory)
		PROP_DROP(ConsoleDropMips)
		PROP_DROP(bStreamable)
		PROP_DROP(CachedBulkDataSize)	//?? serialized twice?
		PROP_DROP(BestTextureInstanceWeight)
		PROP_DROP(SourcePath)
		PROP_DROP(Keywords)
		PROP_DROP(LastModifiedTime_LoInt)
		PROP_DROP(LastModifiedTime_HiInt)
		PROP_DROP(LastModifiedByUser)
#endif // BIOSHOCK
	END_PROP_TABLE

#if RENDERING
	virtual void Bind();
	virtual void GetParams(CMaterialParams &Params) const;
	virtual void SetupGL(unsigned PolyFlags);
	virtual void Release();
	virtual bool IsTranslucent() const;
#endif // RENDERING
};


enum EOutputBlending
{
	OB_Normal,
	OB_Masked,
	OB_Modulate,
	OB_Translucent,
	OB_Invisible,
	OB_Brighten,
	OB_Darken
};

_ENUM(EOutputBlending)
{
	_E(OB_Normal),
	_E(OB_Masked),
	_E(OB_Modulate),
	_E(OB_Translucent),
	_E(OB_Invisible),
	_E(OB_Brighten),
	_E(OB_Darken)
};


class UShader : public URenderedMaterial
{
	DECLARE_CLASS(UShader, URenderedMaterial);
public:
	UMaterial		*Diffuse;
#if BIOSHOCK
	UMaterial		*NormalMap;
#endif
	UMaterial		*Opacity;
	UMaterial		*Specular;
	UMaterial		*SpecularityMask;
	UMaterial		*SelfIllumination;
	UMaterial		*SelfIlluminationMask;
	UMaterial		*Detail;
	float			DetailScale;
	EOutputBlending	OutputBlending;
	bool			TwoSided;
	bool			Wireframe;
	bool			ModulateStaticLighting2X;
	bool			PerformLightingOnSpecularPass;
	bool			ModulateSpecular2X;
#if LINEAGE2
	bool			TreatAsTwoSided;			// strange ...
	bool			ZWrite;
	bool			AlphaTest;
	byte			AlphaRef;
#endif // LINEAGE2
#if BIOSHOCK
	FMaskMaterial	Opacity_Bio;
	FMaskMaterial	HeightMap;
	FMaskMaterial	SpecularMask;
	FMaskMaterial	GlossinessMask;
	FMaskMaterial	ReflectionMask;
	FMaskMaterial	EmissiveMask;
	FMaskMaterial	SubsurfaceMask;
	FMaskMaterial	ClipMask;
#endif // BIOSHOCK

	UShader()
	:	ModulateStaticLighting2X(true)
	,	ModulateSpecular2X(false)
	,	DetailScale(8.0f)
	{}

	BEGIN_PROP_TABLE
		PROP_OBJ(Diffuse)
#if BIOSHOCK
		PROP_OBJ(NormalMap)
#endif
		PROP_OBJ(Opacity)
		PROP_OBJ(Specular)
		PROP_OBJ(SpecularityMask)
		PROP_OBJ(SelfIllumination)
		PROP_OBJ(SelfIlluminationMask)
		PROP_OBJ(Detail)
		PROP_FLOAT(DetailScale)
		PROP_ENUM2(OutputBlending, EOutputBlending)
		PROP_BOOL(TwoSided)
		PROP_BOOL(Wireframe)
		PROP_BOOL(ModulateStaticLighting2X)
		PROP_BOOL(PerformLightingOnSpecularPass)
		PROP_BOOL(ModulateSpecular2X)
#if LINEAGE2
		PROP_BOOL(TreatAsTwoSided)
		PROP_BOOL(ZWrite)
		PROP_BOOL(AlphaTest)
		PROP_BYTE(AlphaRef)
#endif // LINEAGE2
#if BIOSHOCK
		PROP_STRUC(Opacity_Bio,    FMaskMaterial)
		PROP_STRUC(HeightMap,      FMaskMaterial)
		PROP_STRUC(SpecularMask,   FMaskMaterial)
		PROP_STRUC(GlossinessMask, FMaskMaterial)
		PROP_STRUC(ReflectionMask, FMaskMaterial)
		PROP_STRUC(EmissiveMask,   FMaskMaterial)
		PROP_STRUC(SubsurfaceMask, FMaskMaterial)
		PROP_STRUC(ClipMask,       FMaskMaterial)

		PROP_DROP(Emissive)

		PROP_DROP(DiffuseColor)
		PROP_DROP(EmissiveBrightness)
		PROP_DROP(EmissiveColor)
		PROP_DROP(Glossiness)
		PROP_DROP(ReflectionBrightness)
		PROP_DROP(SpecularColor)
		PROP_DROP(SpecularBrightness)
		PROP_DROP(SpecularCubeMapBrightness)
		PROP_DROP(SpecularColorMap)
		PROP_DROP(UseSpecularCubemaps)
		PROP_DROP(HeightMap)
		PROP_DROP(HeightMapStrength)
		PROP_DROP(Masked)			//??
		PROP_DROP(MaterialVisualType)
		PROP_DROP(DiffuseTextureAnimator)
		PROP_DROP(DiffuseColorAnimator)
		PROP_DROP(OpacityTextureAnimator)
		PROP_DROP(SelfIllumTextureAnimator)
		PROP_DROP(SelfIllumColorAnimator)
		PROP_DROP(Subsurface)
		PROP_DROP(SubsurfaceColor2x)
		PROP_DROP(DistortionStrength)
		PROP_DROP(ForceTransparentSorting)
		PROP_DROP(MaxAlphaClipValue)
		PROP_DROP(MinAlphaClipValue)
#endif // BIOSHOCK
	END_PROP_TABLE

#if BIOSHOCK
	virtual void Serialize(FArchive &Ar)
	{
		guard(UShader::Serialize);
		Super::Serialize(Ar);
		TRIBES_HDR(Ar, 0x29);
		unguard;
	}
#endif

#if RENDERING
	virtual void SetupGL(unsigned PolyFlags);
	virtual bool IsTranslucent() const;
	virtual void GetParams(CMaterialParams &Params) const;
#endif
};


class UModifier : public UMaterial
{
	DECLARE_CLASS(UModifier, UMaterial);
public:
	UMaterial		*Material;

	BEGIN_PROP_TABLE
		PROP_OBJ(Material)
	END_PROP_TABLE

#if RENDERING
	virtual void SetupGL(unsigned PolyFlags);
	virtual bool IsTranslucent() const
	{
		return Material ? Material->IsTranslucent() : false;
	}
	virtual void GetParams(CMaterialParams &Params) const;
#endif
};


//?? NOTE: Bioshock EFrameBufferBlending is used for UShader and UFinalBlend, plus it has different values
enum EFrameBufferBlending
{
	FB_Overwrite,
	FB_Modulate,
	FB_AlphaBlend,
	FB_AlphaModulate_MightNotFogCorrectly,
	FB_Translucent,
	FB_Darken,
	FB_Brighten,
	FB_Invisible,
#if LINEAGE2	//?? not needed
	FB_Add,
	FB_InWaterBlend,
	FB_Capture,
#endif
};

_ENUM(EFrameBufferBlending)
{
	_E(FB_Overwrite),
	_E(FB_Modulate),
	_E(FB_AlphaBlend),
	_E(FB_AlphaModulate_MightNotFogCorrectly),
	_E(FB_Translucent),
	_E(FB_Darken),
	_E(FB_Brighten),
	_E(FB_Invisible),
#if LINEAGE2	//?? not needed
	_E(FB_Add),
	_E(FB_InWaterBlend),
	_E(FB_Capture),
#endif
};

class UFinalBlend : public UModifier
{
	DECLARE_CLASS(UFinalBlend, UModifier);
public:
	EFrameBufferBlending FrameBufferBlending;
	bool			ZWrite;
	bool			ZTest;
	bool			AlphaTest;
	bool			TwoSided;
	byte			AlphaRef;
#if LINEAGE2
	bool			TreatAsTwoSided;			// strange ...
#endif

	UFinalBlend()
	:	FrameBufferBlending(FB_Overwrite)
	,	ZWrite(true)
	,	ZTest(true)
	,	TwoSided(false)
	{}
	BEGIN_PROP_TABLE
		PROP_ENUM2(FrameBufferBlending, EFrameBufferBlending)
		PROP_BOOL(ZWrite)
		PROP_BOOL(ZTest)
		PROP_BOOL(AlphaTest)
		PROP_BOOL(TwoSided)
		PROP_BYTE(AlphaRef)
#if LINEAGE2
		PROP_BOOL(TreatAsTwoSided)
#endif
	END_PROP_TABLE

#if RENDERING
	virtual void SetupGL(unsigned PolyFlags);
	virtual bool IsTranslucent() const;
#endif
};


enum EColorOperation
{
	CO_Use_Color_From_Material1,
	CO_Use_Color_From_Material2,
	CO_Multiply,
	CO_Add,
	CO_Subtract,
	CO_AlphaBlend_With_Mask,
	CO_Add_With_Mask_Modulation,
	CO_Use_Color_From_Mask,
};

_ENUM(EColorOperation)
{
	_E(CO_Use_Color_From_Material1),
	_E(CO_Use_Color_From_Material2),
	_E(CO_Multiply),
	_E(CO_Add),
	_E(CO_Subtract),
	_E(CO_AlphaBlend_With_Mask),
	_E(CO_Add_With_Mask_Modulation),
	_E(CO_Use_Color_From_Mask)
};

enum EAlphaOperation
{
	AO_Use_Mask,
	AO_Multiply,
	AO_Add,
	AO_Use_Alpha_From_Material1,
	AO_Use_Alpha_From_Material2,
};

_ENUM(EAlphaOperation)
{
	_E(AO_Use_Mask),
	_E(AO_Multiply),
	_E(AO_Add),
	_E(AO_Use_Alpha_From_Material1),
	_E(AO_Use_Alpha_From_Material2),
};

class UCombiner : public UMaterial
{
	DECLARE_CLASS(UCombiner, UMaterial);
public:
	EColorOperation	CombineOperation;
	EAlphaOperation	AlphaOperation;
	UMaterial		*Material1;
	UMaterial		*Material2;
	UMaterial		*Mask;
	bool			InvertMask;
	bool			Modulate2X;
	bool			Modulate4X;

	UCombiner()
	:	AlphaOperation(AO_Use_Mask)
	{}

	BEGIN_PROP_TABLE
		PROP_ENUM2(CombineOperation, EColorOperation)
		PROP_ENUM2(AlphaOperation, EAlphaOperation)
		PROP_OBJ(Material1)
		PROP_OBJ(Material2)
		PROP_OBJ(Mask)
		PROP_BOOL(InvertMask)
		PROP_BOOL(Modulate2X)
		PROP_BOOL(Modulate4X)
	END_PROP_TABLE

#if RENDERING
	virtual void GetParams(CMaterialParams &Params) const;
	virtual bool IsTranslucent() const
	{
		return false;
	}
#endif
};


enum ETexCoordSrc
{
	TCS_Stream0,
	TCS_Stream1,
	TCS_Stream2,
	TCS_Stream3,
	TCS_Stream4,
	TCS_Stream5,
	TCS_Stream6,
	TCS_Stream7,
	TCS_WorldCoords,
	TCS_CameraCoords,
	TCS_WorldEnvMapCoords,
	TCS_CameraEnvMapCoords,
	TCS_ProjectorCoords,
	TCS_NoChange,				// don't specify a source, just modify it
};

_ENUM(ETexCoordSrc)
{
	_E(TCS_Stream0),
	_E(TCS_Stream1),
	_E(TCS_Stream2),
	_E(TCS_Stream3),
	_E(TCS_Stream4),
	_E(TCS_Stream5),
	_E(TCS_Stream6),
	_E(TCS_Stream7),
	_E(TCS_WorldCoords),
	_E(TCS_CameraCoords),
	_E(TCS_WorldEnvMapCoords),
	_E(TCS_CameraEnvMapCoords),
	_E(TCS_ProjectorCoords),
	_E(TCS_NoChange),
};

enum ETexCoordCount
{
	TCN_2DCoords,
	TCN_3DCoords,
	TCN_4DCoords
};

_ENUM(ETexCoordCount)
{
	_E(TCN_2DCoords),
	_E(TCN_3DCoords),
	_E(TCN_4DCoords)
};

class UTexModifier : public UModifier
{
	DECLARE_CLASS(UTexModifier, UModifier);
public:
	ETexCoordSrc	TexCoordSource;
	ETexCoordCount	TexCoordCount;
	bool			TexCoordProjected;

	UTexModifier()
	:	TexCoordSource(TCS_NoChange)
	,	TexCoordCount(TCN_2DCoords)
	{}

	BEGIN_PROP_TABLE
		PROP_ENUM2(TexCoordSource, ETexCoordSrc)
		PROP_ENUM2(TexCoordCount, ETexCoordCount)
		PROP_BOOL(TexCoordProjected)
	END_PROP_TABLE
};


enum ETexEnvMapType
{
	EM_WorldSpace,
	EM_CameraSpace,
};

_ENUM(ETexEnvMapType)
{
	_E(EM_WorldSpace),
	_E(EM_CameraSpace),
};

class UTexEnvMap : public UTexModifier
{
	DECLARE_CLASS(UTexEnvMap, UTexModifier)
public:
	ETexEnvMapType	EnvMapType;

	UTexEnvMap()
	:	EnvMapType(EM_CameraSpace)
	{
		TexCoordCount = TCN_3DCoords;
	}

	BEGIN_PROP_TABLE
		PROP_ENUM2(EnvMapType, ETexEnvMapType)
	END_PROP_TABLE
};


enum ETexOscillationType
{
	OT_Pan,
	OT_Stretch,
	OT_StretchRepeat,
	OT_Jitter,
};

_ENUM(ETexOscillationType)
{
	_E(OT_Pan),
	_E(OT_Stretch),
	_E(OT_StretchRepeat),
	_E(OT_Jitter),
};

class UTexOscillator : public UTexModifier
{
	DECLARE_CLASS(UTexOscillator, UTexModifier);
public:
	float			UOscillationRate;
	float			VOscillationRate;
	float			UOscillationPhase;
	float			VOscillationPhase;
	float			UOscillationAmplitude;
	float			VOscillationAmplitude;
	ETexOscillationType UOscillationType;
	ETexOscillationType VOscillationType;
	float			UOffset;
	float			VOffset;
	// transient variables
//	FMatrix			M;
//	float			CurrentUJitter;
//	float			CurrentVJitter;

	BEGIN_PROP_TABLE
		PROP_FLOAT(UOscillationRate)
		PROP_FLOAT(VOscillationRate)
		PROP_FLOAT(UOscillationPhase)
		PROP_FLOAT(VOscillationPhase)
		PROP_FLOAT(UOscillationAmplitude)
		PROP_FLOAT(VOscillationAmplitude)
		PROP_ENUM2(UOscillationType, ETexOscillationType)
		PROP_ENUM2(VOscillationType, ETexOscillationType)
		PROP_FLOAT(UOffset)
		PROP_FLOAT(VOffset)
		PROP_DROP(M)
		PROP_DROP(CurrentUJitter)
		PROP_DROP(CurrentVJitter)
	END_PROP_TABLE

	UTexOscillator()
	:	UOscillationRate(1)
	,	VOscillationRate(1)
	,	UOscillationAmplitude(0.1f)
	,	VOscillationAmplitude(0.1f)
	,	UOscillationType(OT_Pan)
	,	VOscillationType(OT_Pan)
	{}
};


class UTexPanner : public UTexModifier
{
	DECLARE_CLASS(UTexPanner, UTexModifier);
public:
	FRotator		PanDirection;
	float			PanRate;
//	FMatrix			M;

	BEGIN_PROP_TABLE
		PROP_ROTATOR(PanDirection)
		PROP_FLOAT(PanRate)
		PROP_DROP(M)
	END_PROP_TABLE

	UTexPanner()
	:	PanRate(0.1f)
	{}
};


enum ETexRotationType
{
	TR_FixedRotation,
	TR_ConstantlyRotating,
	TR_OscillatingRotation,
};

_ENUM(ETexRotationType)
{
	_E(TR_FixedRotation),
	_E(TR_ConstantlyRotating),
	_E(TR_OscillatingRotation),
};

class UTexRotator : public UTexModifier
{
	DECLARE_CLASS(UTexRotator, UTexModifier);
public:
	ETexRotationType TexRotationType;
	FRotator		Rotation;
	float			UOffset;
	float			VOffset;
	FRotator		OscillationRate;
	FRotator		OscillationAmplitude;
	FRotator		OscillationPhase;
//	FMatrix			M;

	BEGIN_PROP_TABLE
		PROP_ENUM2(TexRotationType, ETexRotationType)
		PROP_ROTATOR(Rotation)
		PROP_FLOAT(UOffset)
		PROP_FLOAT(VOffset)
		PROP_ROTATOR(OscillationRate)
		PROP_ROTATOR(OscillationAmplitude)
		PROP_ROTATOR(OscillationPhase)
		PROP_DROP(M)
	END_PROP_TABLE

	UTexRotator()
	:	TexRotationType(TR_FixedRotation)
	{}
};


class UTexScaler : public UTexModifier
{
	DECLARE_CLASS(UTexScaler, UTexModifier)
public:
	float			UScale;
	float			VScale;
	float			UOffset;
	float			VOffset;
//	FMatrix			M;

	BEGIN_PROP_TABLE
		PROP_FLOAT(UScale)
		PROP_FLOAT(VScale)
		PROP_FLOAT(UOffset)
		PROP_FLOAT(VOffset)
		PROP_DROP(M)
	END_PROP_TABLE

	UTexScaler()
	:	UScale(1.0f)
	,	VScale(1.0f)
	{}
};


#if BIOSHOCK

class UFacingShader : public URenderedMaterial
{
	DECLARE_CLASS(UFacingShader, URenderedMaterial);
public:
	UMaterial		*EdgeDiffuse;
	UMaterial		*FacingDiffuse;
	FMaskMaterial	EdgeOpacity;
	FMaskMaterial	FacingOpacity;
	float			EdgeOpacityScale;
	float			FacingOpacityScale;
	UMaterial		*NormalMap;
	FColor			EdgeDiffuseColor;
	FColor			FacingDiffuseColor;
	FColor			EdgeSpecularColor;
	FColor			FacingSpecularColor;
	FMaskMaterial	FacingSpecularMask;
	UMaterial		*FacingSpecularColorMap;
	FMaskMaterial	FacingGlossinessMask;
	FMaskMaterial	EdgeSpecularMask;
	UMaterial		*EdgeSpecularColorMap;
	FMaskMaterial	EdgeGlossinessMask;
	UMaterial		*EdgeEmissive;
	UMaterial		*FacingEmissive;
	FMaskMaterial	EdgeEmissiveMask;
	FMaskMaterial	FacingEmissiveMask;
	float			EdgeEmissiveBrightness;
	float			FacingEmissiveBrightness;
	FColor			EdgeEmissiveColor;
	FColor			FacingEmissiveColor;
	EFrameBufferBlending OutputBlending;
	bool			Masked;
	bool			TwoSided;
	FMaskMaterial	SubsurfaceMask;
	FMaskMaterial	NoiseMask;

	UFacingShader()
	//?? unknown default properties
	{}

	BEGIN_PROP_TABLE
		PROP_OBJ(EdgeDiffuse)
		PROP_OBJ(FacingDiffuse)
		PROP_STRUC(EdgeOpacity, FMaskMaterial)
		PROP_STRUC(FacingOpacity, FMaskMaterial)
		PROP_FLOAT(EdgeOpacityScale)
		PROP_FLOAT(FacingOpacityScale)
		PROP_OBJ(NormalMap)
		PROP_COLOR(EdgeDiffuseColor)
		PROP_COLOR(FacingDiffuseColor)
		PROP_COLOR(EdgeSpecularColor)
		PROP_COLOR(FacingSpecularColor)
		PROP_STRUC(FacingSpecularMask, FMaskMaterial)
		PROP_OBJ(FacingSpecularColorMap)
		PROP_STRUC(FacingGlossinessMask, FMaskMaterial)
		PROP_STRUC(EdgeSpecularMask, FMaskMaterial)
		PROP_OBJ(EdgeSpecularColorMap)
		PROP_STRUC(EdgeGlossinessMask, FMaskMaterial)
		PROP_OBJ(EdgeEmissive)
		PROP_OBJ(FacingEmissive)
		PROP_STRUC(EdgeEmissiveMask, FMaskMaterial)
		PROP_STRUC(FacingEmissiveMask, FMaskMaterial)
		PROP_FLOAT(EdgeEmissiveBrightness)
		PROP_FLOAT(FacingEmissiveBrightness)
		PROP_COLOR(EdgeEmissiveColor)
		PROP_COLOR(FacingEmissiveColor)
		PROP_ENUM2(OutputBlending, EFrameBufferBlending)
		PROP_BOOL(Masked)
		PROP_BOOL(TwoSided)
		PROP_STRUC(SubsurfaceMask, FMaskMaterial)
		PROP_STRUC(NoiseMask, FMaskMaterial)
		PROP_DROP(EdgeDiffuseTextureAnimator)
		PROP_DROP(FacingDiffuseTextureAnimator)
		PROP_DROP(EdgeDiffuseColorAnimator)
		PROP_DROP(FacingDiffuseColorAnimator)
		PROP_DROP(EdgeOpacityTextureAnimator)
		PROP_DROP(FacingOpacityTextureAnimator)
		PROP_DROP(BlendTextureAnimator)
		PROP_DROP(NormalTextureAnimator)
		PROP_DROP(EdgeSelfIllumColorAnimator)
		PROP_DROP(FacingSelfIllumColorAnimator)
		PROP_DROP(FacingSpecularAnimator)
		PROP_DROP(EdgeSpecularAnimator)
		PROP_DROP(EdgeGlossiness)
		PROP_DROP(FacingGlossiness)
		PROP_DROP(EdgeSpecularBrightness)
		PROP_DROP(FacingSpecularBrightness)
		PROP_DROP(Subsurface)
		PROP_DROP(SubsurfaceColor2x)
		PROP_DROP(ForceTransparentSorting)
		PROP_DROP(Hardness)
	END_PROP_TABLE

#if RENDERING
	virtual void SetupGL(unsigned PolyFlags);
	virtual bool IsTranslucent() const;
	virtual void GetParams(CMaterialParams &Params) const;
#endif
};

#endif // BIOSHOCK


#if UNREAL3

/*-----------------------------------------------------------------------------
	Unreal Engine 3 materials
-----------------------------------------------------------------------------*/

class UTexture3 : public UUnrealMaterial	// in UE3 it is derived from USurface->UObject; real name is UTexture
{
	DECLARE_CLASS(UTexture3, UUnrealMaterial)
public:
	float			UnpackMin[4];
	float			UnpackMax[4];
	FByteBulkData	SourceArt;

	UTexture3()
	{
		UnpackMax[0] = UnpackMax[1] = UnpackMax[2] = UnpackMax[3] = 1.0f;
	}

	BEGIN_PROP_TABLE
		PROP_FLOAT(UnpackMin)
		PROP_FLOAT(UnpackMax)
		// no properties required (all are for importing and cooking)
		PROP_DROP(SRGB)
		PROP_DROP(RGBE)
		PROP_DROP(CompressionNoAlpha)
		PROP_DROP(CompressionNone)
		PROP_DROP(CompressionNoMipmaps)
		PROP_DROP(CompressionFullDynamicRange)
		PROP_DROP(DeferCompression)
		PROP_DROP(NeverStream)
		PROP_DROP(bDitherMipMapAlpha)
		PROP_DROP(bPreserveBorderR)
		PROP_DROP(bPreserveBorderG)
		PROP_DROP(bPreserveBorderB)
		PROP_DROP(bPreserveBorderA)
		PROP_DROP(CompressionSettings)
		PROP_DROP(Filter)
		PROP_DROP(LODGroup)
		PROP_DROP(LODBias)
		PROP_DROP(SourceFilePath)
		PROP_DROP(SourceFileTimestamp)
		PROP_DROP(LightingGuid)
		PROP_DROP(AdjustRGBCurve)			// UDK
		PROP_DROP(AdjustSaturation)			// UDK
		PROP_DROP(AdjustBrightnessCurve)	// UDK
#if HUXLEY
		PROP_DROP(SourceArtWidth)
		PROP_DROP(SourceArtHeight)
#endif // HUXLEY
#if A51
		PROP_DROP(LODBiasWindows)
#endif // A51
#if BATMAN
		PROP_DROP(ForceOldCompression)
#endif // BATMAN
	END_PROP_TABLE

	virtual void Serialize(FArchive &Ar)
	{
		guard(UTexture3::Serialize);
		Super::Serialize(Ar);
#if TRANSFORMERS
		// Transformers: SourceArt is moved to separate class; but The Bourne Conspiracy has it (real ArLicenseeVer is unknown)
		if (Ar.Game == GAME_Transformers && Ar.ArLicenseeVer >= 100) return;
#endif
#if APB
		if (Ar.Game == GAME_APB)
		{
			// APB has source art stored in separate bulk
			// here are 2 headers: 1 for SourceArt and 1 for TIndirectArray<BulkData> MipSourceArt
			// we can skip these blocks by skipping headers
			// each header is a magic 0x5D0E7707 + position in package
			Ar.Seek(Ar.Tell() + 8 * 2);
			return;
		}
#endif // APB
		SourceArt.Serialize(Ar);
		unguard;
	}

#if RENDERING
	virtual bool IsTexture() const
	{
		return true;
	}
#endif // RENDERING
};

enum EPixelFormat
{
	PF_Unknown,
	PF_A32B32G32R32F,
	PF_A8R8G8B8,
	PF_G8,
	PF_G16,
	PF_DXT1,
	PF_DXT3,
	PF_DXT5,
	PF_UYVY,
	PF_FloatRGB,			// A RGB FP format with platform-specific implementation, for use with render targets
	PF_FloatRGBA,			// A RGBA FP format with platform-specific implementation, for use with render targets
	PF_DepthStencil,		// A depth+stencil format with platform-specific implementation, for use with render targets
	PF_ShadowDepth,			// A depth format with platform-specific implementation, for use with render targets
	PF_FilteredShadowDepth, // A depth format with platform-specific implementation, that can be filtered by hardware
	PF_R32F,
	PF_G16R16,
	PF_G16R16F,
	PF_G16R16F_FILTER,
	PF_G32R32F,
	PF_A2B10G10R10,
	PF_A16B16G16R16,
	PF_D24,
	PF_R16F,
	PF_R16F_FILTER,
	PF_BC5,
	PF_V8U8,
	PF_A1,
#if MASSEFF
	PF_NormalMap_LQ,
	PF_NormalMap_HQ,
#endif
};

_ENUM(EPixelFormat)
{
	_E(PF_Unknown),
	_E(PF_A32B32G32R32F),
	_E(PF_A8R8G8B8),
	_E(PF_G8),
	_E(PF_G16),
	_E(PF_DXT1),
	_E(PF_DXT3),
	_E(PF_DXT5),
	_E(PF_UYVY),
	_E(PF_FloatRGB),
	_E(PF_FloatRGBA),
	_E(PF_DepthStencil),
	_E(PF_ShadowDepth),
	_E(PF_FilteredShadowDepth),
	_E(PF_R32F),
	_E(PF_G16R16),
	_E(PF_G16R16F),
	_E(PF_G16R16F_FILTER),
	_E(PF_G32R32F),
	_E(PF_A2B10G10R10),
	_E(PF_A16B16G16R16),
	_E(PF_D24),
	_E(PF_R16F),
	_E(PF_R16F_FILTER),
	_E(PF_BC5),
	_E(PF_V8U8),
	_E(PF_A1),
#if MASSEFF
	_E(PF_NormalMap_LQ),
	_E(PF_NormalMap_HQ),
#endif
};

enum ETextureFilter
{
	TF_Nearest,
	TF_Linear
};

enum ETextureAddress
{
	TA_Wrap,
	TA_Clamp,
	TA_Mirror
};

_ENUM(ETextureAddress)
{
	_E(TA_Wrap),
	_E(TA_Clamp),
	_E(TA_Mirror)
};

struct FTexture2DMipMap
{
	FByteBulkData	Data;	// FTextureMipBulkData
	int				SizeX;
	int				SizeY;

	friend FArchive& operator<<(FArchive &Ar, FTexture2DMipMap &Mip)
	{
		Mip.Data.Serialize(Ar);
#if DARKVOID
		if (Ar.Game == GAME_DarkVoid)
		{
			FByteBulkData DataX360Gamma;
			DataX360Gamma.Serialize(Ar);
		}
#endif // DARKVOID
		return Ar << Mip.SizeX << Mip.SizeY;
	}
};

class UTexture2D : public UTexture3
{
	DECLARE_CLASS(UTexture2D, UTexture3)
public:
	TArray<FTexture2DMipMap> Mips;
	int				SizeX;
	int				SizeY;
	EPixelFormat	Format;
	ETextureAddress	AddressX;
	ETextureAddress	AddressY;
	FName			TextureFileCacheName;
	FGuid			TextureFileCacheGuid;
	int				MipTailBaseIdx;
	bool			bForcePVRTC4;		// iPhone

#if RENDERING
	// rendering implementation fields
	GLuint			TexNum;
#endif

#if RENDERING
	UTexture2D()
	:	TexNum(0)
	,	Format(PF_Unknown)
	,	AddressX(TA_Wrap)
	,	AddressY(TA_Wrap)
	,	bForcePVRTC4(false)
	{
		TextureFileCacheName.Str = "None";
	}
#endif

	BEGIN_PROP_TABLE
		PROP_INT(SizeX)
		PROP_INT(SizeY)
		PROP_ENUM2(Format, EPixelFormat)
		PROP_ENUM2(AddressX, ETextureAddress)
		PROP_ENUM2(AddressY, ETextureAddress)
		PROP_NAME(TextureFileCacheName)
		PROP_INT(MipTailBaseIdx)
		// drop unneeded props
		PROP_DROP(bGlobalForceMipLevelsToBeResident)
		PROP_DROP(OriginalSizeX)
		PROP_DROP(OriginalSizeY)
		PROP_BOOL(bForcePVRTC4)
#if FRONTLINES
		PROP_DROP(NumMips)
		PROP_DROP(SourceDataSizeX)
		PROP_DROP(SourceDataSizeY)
#endif // FRONTLINES
#if MASSEFF
		PROP_DROP(TFCFileGuid)
#endif
	END_PROP_TABLE

	virtual void Serialize(FArchive &Ar)
	{
		guard(UTexture2D::Serialize);
		Super::Serialize(Ar);
#if TERA
		if (Ar.Game == GAME_Tera && Ar.ArLicenseeVer >= 3)
		{
			FString SourceFilePath; // field from UTexture
			Ar << SourceFilePath;
		}
#endif // TERA
		if (Ar.ArVer < 297)
		{
			int Format2;
			Ar << SizeX << SizeY << Format2;
			Format = (EPixelFormat)Format2;		// int -> byte (enum)
		}
#if BORDERLANDS
		if (Ar.Game == GAME_Borderlands) Ar.Seek(Ar.Tell() + 16);	// some hash
#endif
		Ar << Mips;
#if BORDERLANDS
		if (Ar.Game == GAME_Borderlands) Ar.Seek(Ar.Tell() + 16);	// some hash
#endif
#if MASSEFF
		if (Ar.Game == GAME_MassEffect && Ar.ArLicenseeVer >= 65)
		{
			int unkFC;
			Ar << unkFC;
		}
#endif // MASSEFF
#if HUXLEY
		if (Ar.Game == GAME_Huxley) goto skip_rest_quiet;
#endif
#if DCU_ONLINE
		if (Ar.Game == GAME_DCUniverse && (Ar.ArLicenseeVer & 0xFF00) >= 0x1700) return;
#endif
		if (Ar.ArVer >= 567)
			Ar << TextureFileCacheGuid;
		if (Ar.ArVer >= 674)
		{
			TArray<FTexture2DMipMap> CachedPVRTCMips;
			Ar << CachedPVRTCMips;
		}

		// some hack to support more games ...
		if (Ar.Tell() < Ar.GetStopper())
		{
			printf("UTexture2D %s: dropping %d bytes\n", Name, Ar.GetStopper() - Ar.Tell());
		skip_rest_quiet:
			Ar.Seek(Ar.GetStopper());
		}

		unguard;
	}

	bool LoadBulkTexture(int MipIndex) const;
	virtual byte *Decompress(int &USize, int &VSize) const;
#if RENDERING
	virtual void Bind();
	virtual void GetParams(CMaterialParams &Params) const;
	virtual void Release();
#endif
};


class ULightMapTexture2D : public UTexture2D
{
	DECLARE_CLASS(ULightMapTexture2D, UTexture2D)
public:
	virtual void Serialize(FArchive &Ar)
	{
		Super::Serialize(Ar);
		Ar.Seek(Ar.GetStopper());
	}
};


class UTextureCube : public UTexture3
{
	DECLARE_CLASS(UTextureCube, UTexture3)
public:
	UTexture2D		*FacePosX;
	UTexture2D		*FaceNegX;
	UTexture2D		*FacePosY;
	UTexture2D		*FaceNegY;
	UTexture2D		*FacePosZ;
	UTexture2D		*FaceNegZ;

#if RENDERING
	// rendering implementation fields
	GLuint			TexNum;
#endif

	BEGIN_PROP_TABLE
		PROP_OBJ(FacePosX)
		PROP_OBJ(FaceNegX)
		PROP_OBJ(FacePosY)
		PROP_OBJ(FaceNegY)
		PROP_OBJ(FacePosZ)
		PROP_OBJ(FaceNegZ)
	END_PROP_TABLE

#if RENDERING
	virtual void Bind();
	virtual void GetParams(CMaterialParams &Params) const;
	virtual void Release();
	virtual bool IsTextureCube() const
	{
		return true;
	}
#endif // RENDERING
};


enum EBlendMode
{
	BLEND_Opaque,
	BLEND_Masked,
	BLEND_Translucent,
	BLEND_Additive,
	BLEND_Modulate
};

_ENUM(EBlendMode)
{
	_E(BLEND_Opaque),
	_E(BLEND_Masked),
	_E(BLEND_Translucent),
	_E(BLEND_Additive),
	_E(BLEND_Modulate)
};

enum EMaterialLightingModel	// unused now
{
	MLM_Phong,
	MLM_NonDirectional,
	MLM_Unlit,
	MLM_SHPRT,
	MLM_Custom
};

enum EMobileSpecularMask
{
	MSM_Constant,
	MSM_Luminance,
	MSM_DiffuseRed,
	MSM_DiffuseGreen,
	MSM_DiffuseBlue,
	MSM_DiffuseAlpha,
	MSM_MaskTextureRGB
};

_ENUM(EMobileSpecularMask)
{
	_E(MSM_Constant),
	_E(MSM_Luminance),
	_E(MSM_DiffuseRed),
	_E(MSM_DiffuseGreen),
	_E(MSM_DiffuseBlue),
	_E(MSM_DiffuseAlpha),
	_E(MSM_MaskTextureRGB)
};


class UMaterialInterface : public UUnrealMaterial
{
	DECLARE_CLASS(UMaterialInterface, UUnrealMaterial)
public:
	// Mobile
	UTexture3		*FlattenedTexture;
	UTexture3		*MobileBaseTexture;
	UTexture3		*MobileNormalTexture;
	bool			bUseMobileSpecular;
	float			MobileSpecularPower;
	EMobileSpecularMask MobileSpecularMask;
	UTexture3		*MobileMaskTexture;

	UMaterialInterface()
	:	MobileSpecularPower(16)
//	,	MobileSpecularMask(MSM_Constant)
	{}

	BEGIN_PROP_TABLE
		PROP_DROP(PreviewMesh)
		PROP_DROP(LightingGuid)
		// Mobile
		PROP_OBJ(FlattenedTexture)
		PROP_OBJ(MobileBaseTexture)
		PROP_OBJ(MobileNormalTexture)
		PROP_DROP(bMobileAllowFog)
		PROP_BOOL(bUseMobileSpecular)
		PROP_DROP(MobileSpecularColor)
		PROP_DROP(bUseMobilePixelSpecular)
		PROP_FLOAT(MobileSpecularPower)
		PROP_ENUM2(MobileSpecularMask, EMobileSpecularMask)
		PROP_OBJ(MobileMaskTexture)
#if MASSEFF
		PROP_DROP(m_Guid)
#endif
	END_PROP_TABLE

#if RENDERING
	virtual void GetParams(CMaterialParams &Params) const;
#endif
};


class UMaterial3 : public UMaterialInterface
{
	DECLARE_CLASS(UMaterial3, UMaterialInterface)
public:
	bool			TwoSided;
	bool			bDisableDepthTest;
	bool			bIsMasked;
	EBlendMode		BlendMode;
	float			OpacityMaskClipValue;
	TArray<UTexture3*> ReferencedTextures;

	UMaterial3()
	:	OpacityMaskClipValue(0.333f)		//?? check
	,	BlendMode(BLEND_Opaque)
	{}

	BEGIN_PROP_TABLE
		PROP_BOOL(TwoSided)
		PROP_BOOL(bDisableDepthTest)
		PROP_BOOL(bIsMasked)
		PROP_ARRAY(ReferencedTextures, UObject*)
		PROP_ENUM2(BlendMode, EBlendMode)
		PROP_FLOAT(OpacityMaskClipValue)
		//!! should be used (main material inputs in UE3 material editor)
		PROP_DROP(DiffuseColor)
		PROP_DROP(DiffusePower)				// GoW2
		PROP_DROP(EmissiveColor)
		PROP_DROP(SpecularColor)
		PROP_DROP(SpecularPower)
		PROP_DROP(Opacity)
		PROP_DROP(OpacityMask)
		PROP_DROP(Distortion)
		PROP_DROP(TwoSidedLightingMask)		// TransmissionMask ?
		PROP_DROP(TwoSidedLightingColor)	// TransmissionColor ?
		PROP_DROP(Normal)
		PROP_DROP(CustomLighting)
		// drop other props
		PROP_DROP(PhysMaterial)
		PROP_DROP(PhysicalMaterial)
		PROP_DROP(LightingModel)			//!! use it (EMaterialLightingModel)
		// usage
		PROP_DROP(bUsedAsLightFunction)
		PROP_DROP(bUsedWithFogVolumes)
		PROP_DROP(bUsedAsSpecialEngineMaterial)
		PROP_DROP(bUsedWithSkeletalMesh)
		PROP_DROP(bUsedWithParticleSystem)
		PROP_DROP(bUsedWithParticleSprites)
		PROP_DROP(bUsedWithBeamTrails)
		PROP_DROP(bUsedWithParticleSubUV)
		PROP_DROP(bUsedWithFoliage)
		PROP_DROP(bUsedWithSpeedTree)
		PROP_DROP(bUsedWithStaticLighting)
		PROP_DROP(bUsedWithLensFlare)
		PROP_DROP(bUsedWithGammaCorrection)
		PROP_DROP(bUsedWithInstancedMeshParticles)
		PROP_DROP(bUsedWithDecals)			// GoW2
		PROP_DROP(bUsedWithFracturedMeshes)	// GoW2
		PROP_DROP(bUsedWithFluidSurfaces)	// GoW2
		// other
		PROP_DROP(Wireframe)
		PROP_DROP(bIsFallbackMaterial)
		PROP_DROP(FallbackMaterial)			//!! use it
		PROP_DROP(EditorX)
		PROP_DROP(EditorY)
		PROP_DROP(EditorPitch)
		PROP_DROP(EditorYaw)
		PROP_DROP(Expressions)				//!! use it
		PROP_DROP(EditorComments)
		PROP_DROP(EditorCompounds)
		PROP_DROP(bUsesDistortion)
		PROP_DROP(bUsesSceneColor)
		PROP_DROP(bUsedWithMorphTargets)
		PROP_DROP(bAllowFog)
		PROP_DROP(ReferencedTextureGuids)
#if MEDGE
		PROP_DROP(BakerBleedBounceAmount)
		PROP_DROP(BakerAlpha)
#endif // MEDGE
#if TLR
		PROP_DROP(VFXShaderType)
#endif
#if MASSEFF
		PROP_DROP(bUsedWithLightEnvironments)
		PROP_DROP(AllowsEffectsMaterials)
#endif // MASSEFF
	END_PROP_TABLE

	virtual void Serialize(FArchive &Ar)
	{
		Super::Serialize(Ar);
		if (Ar.ArVer >= 656)
		{
			guard(SerializeFMaterialResource);
			// Starting with version 656 UE3 has deprecated ReferencedTextures array.
			// This array is serialized inside FMaterialResource which is not needed
			// for us in other case.
			// FMaterialResource serialization is below
			TArray<FString>			f10;
			TMap<UObject*, int>		f1C;
			int						f58;
			FGuid					f60;
			int						f80;
			Ar << f10 << f1C << f58 << f60 << f80;
			if (Ar.ArVer >= 656) Ar << ReferencedTextures;	// that is ...
			// other fields are not interesting ...
			unguard;
		}
		Ar.Seek(Ar.GetStopper());			//?? drop native data
	}

#if RENDERING
	virtual void SetupGL(unsigned PolyFlags);
	virtual void GetParams(CMaterialParams &Params) const;
	virtual bool IsTranslucent() const;
#endif
};

class UMaterialInstance : public UMaterialInterface
{
	DECLARE_CLASS(UMaterialInstance, UMaterialInterface)
public:
	UUnrealMaterial	*Parent;			// UMaterialInterface*

	BEGIN_PROP_TABLE
		PROP_OBJ(Parent)
		PROP_DROP(PhysMaterial)
		PROP_DROP(bHasStaticPermutationResource)
		PROP_DROP(ReferencedTextures)	// this is a textures from Parent plus own overrided textures
		PROP_DROP(ReferencedTextureGuids)
		PROP_DROP(ParentLightingGuid)
	END_PROP_TABLE

	virtual void Serialize(FArchive &Ar)
	{
		Super::Serialize(Ar);
		Ar.Seek(Ar.GetStopper());			//?? drop native data
	}
};


struct FScalarParameterValue
{
	DECLARE_STRUCT(FScalarParameterValue)

	FName			ParameterName;
	float			ParameterValue;
//	FGuid			ExpressionGUID;

	BEGIN_PROP_TABLE
		PROP_NAME(ParameterName)
		PROP_FLOAT(ParameterValue)
		PROP_DROP(ExpressionGUID)	//!! test nested structure serialization later
	END_PROP_TABLE
};

struct FTextureParameterValue
{
	DECLARE_STRUCT(FTextureParameterValue)

	FName			ParameterName;
	UTexture3		*ParameterValue;
//	FGuid			ExpressionGUID;

	BEGIN_PROP_TABLE
		PROP_NAME(ParameterName)
		PROP_OBJ(ParameterValue)
		PROP_DROP(ExpressionGUID)	//!! test nested structure serialization later
	END_PROP_TABLE
};

struct FVectorParameterValue
{
	DECLARE_STRUCT(FVectorParameterValue)

	FName			ParameterName;
	FLinearColor	ParameterValue;
//	FGuid			ExpressionGUID;

	BEGIN_PROP_TABLE
		PROP_NAME(ParameterName)
		PROP_STRUC(ParameterValue, FLinearColor)
		PROP_DROP(ExpressionGUID)	//!! test nested structure serialization later
	END_PROP_TABLE
};

class UMaterialInstanceConstant : public UMaterialInstance
{
	DECLARE_CLASS(UMaterialInstanceConstant, UMaterialInstance)
public:
	TArray<FScalarParameterValue>	ScalarParameterValues;
	TArray<FTextureParameterValue>	TextureParameterValues;
	TArray<FVectorParameterValue>	VectorParameterValues;

	BEGIN_PROP_TABLE
		PROP_ARRAY(ScalarParameterValues,  FScalarParameterValue )
		PROP_ARRAY(TextureParameterValues, FTextureParameterValue)
		PROP_ARRAY(VectorParameterValues,  FVectorParameterValue )
		PROP_DROP(FontParameterValues)
	END_PROP_TABLE

#if RENDERING
	virtual void SetupGL(unsigned PolyFlags);
	virtual void GetParams(CMaterialParams &Params) const;
	virtual bool IsTranslucent() const
	{
		return Parent ? Parent->IsTranslucent() : false;
	}
#endif
};


#if DCU_ONLINE

struct UIStreamingTexture_Info_DCU
{
	unsigned		Hash;				// real texture has no hash, it is a Map<int,UIStreamingTexture_Info>
	int				nWidth;
	int				nHeight;
	int				BulkDataFlags;
	int				ElementCount;
	int				BulkDataOffsetInFile;
	int				BulkDataSizeOnDisk;
	int				Format;
	byte			bSRGB;
	FName			TextureFileCacheName;

	friend FArchive& operator<<(FArchive &Ar, UIStreamingTexture_Info_DCU &S)
	{
		Ar << S.Hash;					// serialize it in the structure
		return Ar << S.nWidth << S.nHeight << S.BulkDataFlags << S.ElementCount << S.BulkDataOffsetInFile
				  << S.BulkDataSizeOnDisk << S.Format << S.bSRGB << S.TextureFileCacheName;
	}
};

//?? non-functional class, remove it later
class UUIStreamingTextures : public UObject
{
	DECLARE_CLASS(UUIStreamingTextures, UObject);
public:
	TArray<UIStreamingTexture_Info_DCU> TextureHashToInfo;

	// generated data
	TArray<UTexture2D*>		Textures;
	TArray<char*>			Names;
	virtual ~UUIStreamingTextures();

	virtual void Serialize(FArchive &Ar)
	{
		guard(UUIStreamingTextures::Serialize);
		Super::Serialize(Ar);
		Ar << TextureHashToInfo;
		unguard;
	}

	virtual void PostLoad();
};

#endif // DCU_ONLINE

#endif // UNREAL3


#define REGISTER_MATERIAL_CLASSES		\
	REGISTER_CLASS(UConstantColor)		\
	REGISTER_CLASS(UBitmapMaterial)		\
	REGISTER_CLASS(UPalette)			\
	REGISTER_CLASS(UShader)				\
	REGISTER_CLASS(UCombiner)			\
	REGISTER_CLASS(UTexture)			\
	REGISTER_CLASS(UFinalBlend)			\
	REGISTER_CLASS(UTexEnvMap)			\
	REGISTER_CLASS(UTexOscillator)		\
	REGISTER_CLASS(UTexPanner)			\
	REGISTER_CLASS(UTexRotator)			\
	REGISTER_CLASS(UTexScaler)

#define REGISTER_MATERIAL_CLASSES_BIO	\
	REGISTER_CLASS(FMaskMaterial)		\
	REGISTER_CLASS(UFacingShader)

#define REGISTER_MATERIAL_CLASSES_U3	\
	REGISTER_CLASS_ALIAS(UMaterial3, UMaterial) \
	REGISTER_CLASS(UTexture2D)			\
	REGISTER_CLASS(ULightMapTexture2D)	\
	REGISTER_CLASS(UTextureCube)		\
	REGISTER_CLASS(FScalarParameterValue)  \
	REGISTER_CLASS(FTextureParameterValue) \
	REGISTER_CLASS(FVectorParameterValue)  \
	REGISTER_CLASS(UMaterialInstanceConstant)

#define REGISTER_MATERIAL_CLASSES_DCUO	\
	REGISTER_CLASS(UUIStreamingTextures)

#define REGISTER_MATERIAL_ENUMS			\
	REGISTER_ENUM(ETextureFormat)		\
	REGISTER_ENUM(ETexClampMode)		\
	REGISTER_ENUM(EOutputBlending)		\
	REGISTER_ENUM(EFrameBufferBlending)	\
	REGISTER_ENUM(EColorOperation)		\
	REGISTER_ENUM(EAlphaOperation)		\
	REGISTER_ENUM(ETexCoordSrc)			\
	REGISTER_ENUM(ETexCoordCount)		\
	REGISTER_ENUM(ETexEnvMapType)		\
	REGISTER_ENUM(ETexOscillationType)	\
	REGISTER_ENUM(ETexRotationType)

#define REGISTER_MATERIAL_ENUMS_U3		\
	REGISTER_ENUM(EPixelFormat)			\
	REGISTER_ENUM(ETextureAddress)		\
	REGISTER_ENUM(EBlendMode)			\
	REGISTER_ENUM(EMobileSpecularMask)

#endif // __UNMATERIAL_H__
