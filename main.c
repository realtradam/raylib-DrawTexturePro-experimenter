// Credit:
// Raylib and Raygui made by Ramon Santamaria (@raysan5) (https://www.raylib.com)
// kenney.png is made by Kenney (https://www.kenney.nl/assets/simplified-platformer-pack)
// Font is "Liberation Sans"
// Special thanks to Jae (@jdeokkim) for the initial concept
// Code is by _Tradam

#include "raylib.h"
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define CLAMP(val, min, max) MIN(max, MAX(val, min))

const int screenWidth = 900;
const int screenHeight = 600;

const int elementMargin = 10;
const int elementBorderThickness = 3;
const Rectangle elementSliders = {
	.x = 0 + elementMargin,
	.y = 28 + elementMargin,
	.width = screenWidth - (elementMargin * 2),
	.height = (screenHeight / 3) - (elementMargin) - 28
};
const Rectangle elementCode = {
	.x = 0 + elementMargin,
	.y = (screenHeight / 3) + (elementMargin) + 8,
	.width = (screenWidth / 3) - (elementMargin),
	.height = ((2 * screenHeight) / 3) - (elementMargin * 2) - 8
};
const Rectangle elementPreRender = {
	.x = (screenWidth / 3) + (elementMargin / 2),
	.y = (screenHeight / 3) + elementMargin + 8,
	.width = (screenWidth / 3) - (elementMargin),
	.height = ((2 * screenHeight) / 3) - (elementMargin * 2) - 8
};
const Rectangle elementRender = {
	.x = ((2 * screenWidth) / 3),
	.y = (screenHeight / 3) + elementMargin + 8,
	.width = (screenWidth / 3) - (elementMargin),
	.height = ((2 * screenHeight) / 3) - (elementMargin * 2) - 8
};

Font sourceCodeFont;
int fontSize = 14;
int fontSpacing = 0;

enum dragStates{Released, HeldSource, HeldDest, HeldOrigin} dragState = Released;
Vector2 mouseOffset = {.x = 0, .y = 0};

char codePreviewArray[21][66] = {
	"DrawTexturePro(",
	"			texture,",
	"			(Rectangle) { // Source Rectangle",
	"","","","", // 3-6 are filled out dynamically
	"			},",
	"			(Rectangle) { // Dest Rectangle",
	"","","","", // 9-12 are filled out dynamically
	"			},",
	"			(Vector2) { // Origin",
	"","", // 15-16 are filled out dynamically
	"			},",
	"", // 18 is filled out dynamically
	"			WHITE // Color",
	");"
};
int i = 0;
Color codePreviewHighlight[21];

// dtp mean draw texture pro
Rectangle predtpSource;
Rectangle dtpSource = {
	.x = 0,
	.y = 0,
	.width = 48,
	.height = 48
};
Rectangle predtpDest;
Rectangle dtpDest = {
	.x = 0,
	.y = 0,
	.width = 96,
	.height = 96
};
Vector2 predtpOrigin;
Vector2 dtpOrigin = {
	.x = 0,
	.y = 0
};
float predtpRotation;
float dtpRotation = 0;

const int gridSize = 20;
const int gridCenter = 120;

Texture2D sampleSprite;

RenderTexture2D previewElementPre;
RenderTexture2D previewElementResult;

void DrawElementBorders();
void SetupDifference();
void DrawUI();
void ResolveMouseState();
void CheckDifference();
void DrawCodeDisplay();
void DrawOutput();

void HandleDroppedFiles();

void UpdateDrawFrame()
{
	HandleDroppedFiles();

	BeginDrawing();
	ClearBackground(RAYWHITE);

	DrawElementBorders();
	SetupDifference();
	DrawUI();
	ResolveMouseState();
	DrawCodeDisplay();
	CheckDifference();
	DrawOutput();

	EndDrawing();
}

int main()
{
	InitWindow(screenWidth, screenHeight, "DrawTexturePro Example");
	SetTargetFPS(60);
	GuiLoadStyle("assets/style_lavanda.rgs");

	sampleSprite = LoadTexture("assets/kenney.png");

	sourceCodeFont = LoadFontEx("assets/LiberationMono-Regular.ttf", fontSize, 0, 250);
	GuiSetFont(sourceCodeFont);
	GuiSetStyle(DEFAULT, TEXT_SIZE, fontSize);
	GuiSetStyle(DEFAULT, TEXT_SPACING, fontSpacing);

	for(i = 0; i < 21; i += 1) {
		codePreviewHighlight[i] = BEIGE;
		codePreviewHighlight[i].a = 0;
	}

	previewElementPre = LoadRenderTexture(
			elementRender.width - 20,
			elementRender.height - 20
			);
	previewElementResult = LoadRenderTexture(
			elementRender.width - 20,
			elementRender.height - 20
			);

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
	while (!WindowShouldClose())
	{
		UpdateDrawFrame();
	}
#endif
	CloseWindow();

	return 0;
}

void DrawElementBorders()
{
	GuiPanel(
			(Rectangle) {
			.x = 0,
			.y = 0,
			.width = screenWidth,
			.height = screenHeight + 2
			},
			"DrawTexturePro Interactive Demo"
			);
	GuiGroupBox(
			elementSliders,
			"Control Sliders"
			);
	GuiGroupBox(
			elementCode,
			"Corresponding Code"
			);
	DrawRectangle(
			elementCode.x + 10,
			elementCode.y + 10,
			elementCode.width - 20,
			elementCode.height - 20,
			RAYWHITE
			);
	GuiGroupBox(
			elementPreRender,
			"Source Texture"
			);
	DrawRectangle(
			elementPreRender.x + 10,
			elementPreRender.y + 10,
			elementPreRender.width - 20,
			elementPreRender.height - 20,
			RAYWHITE
			);
	GuiGroupBox(
			elementRender,
			"Rendered Texture"
			);
	DrawRectangle(
			elementRender.x + 10,
			elementRender.y + 10,
			elementRender.width - 20,
			elementRender.height - 20,
			RAYWHITE
			);
}


void SetupDifference() {
	predtpSource = dtpSource;
	predtpDest = dtpDest;
	predtpOrigin = dtpOrigin;
	predtpRotation = dtpRotation;
}


void ResolveMouseState() {
	//elementRender elementPreRender

	//if mouse click
	if (IsMouseButtonPressed(0)) {
		// Source Square
		if (
				CheckCollisionPointRec(
					GetMousePosition(),
					(Rectangle) {
					.x = elementPreRender.x + dtpSource.x + 10 + gridCenter,
					.y = elementPreRender.y + dtpSource.y + 10 + gridCenter,
					.width = fabs(dtpSource.width),
					.height = fabs(dtpSource.height)
					}
					)
		   ) {
			GuiLock();
			dragState = HeldSource;
			mouseOffset = GetMousePosition();
			mouseOffset.x -= elementPreRender.x + dtpSource.x + 10 + gridCenter;
			mouseOffset.y -= elementPreRender.y + dtpSource.y + 10 + gridCenter;
		}
		// Origin Dot
		else if (
				CheckCollisionPointCircle(
					GetMousePosition(),
					(Vector2) {
					.x = elementRender.x - dtpOrigin.x + dtpDest.x + 10 + gridCenter,
					.y = elementRender.y - dtpOrigin.y + + dtpDest.y + 10 + gridCenter
					},
					6
					)
				) {
			GuiLock();
			dragState = HeldOrigin;
			mouseOffset = GetMousePosition();
			mouseOffset.x -= elementRender.x - dtpOrigin.x + (2 * dtpDest.x) + 10 + gridCenter;
			mouseOffset.y -= elementRender.y - dtpOrigin.y + (2 * dtpDest.y) + 10 + gridCenter;
		}
		// Dest Square
		else if (
				CheckCollisionPointRec(
					GetMousePosition(),
					(Rectangle) {
					.x = elementRender.x + dtpDest.x + 10 + gridCenter,
					.y = elementRender.y + dtpDest.y + 10 + gridCenter,
					.width = fabs(dtpDest.width),
					.height = fabs(dtpDest.height)
					}
					)
		   ) {
			GuiLock();
			dragState = HeldDest;
			mouseOffset = GetMousePosition();
			mouseOffset.x -= elementRender.x + dtpDest.x + 10 + gridCenter;
			mouseOffset.y -= elementRender.y + dtpDest.y + 10 + gridCenter;
		}
	}
	else if (IsMouseButtonDown(0)) {
		switch (dragState) {
			case HeldSource :
			dtpSource.x = CLAMP(-mouseOffset.x + GetMouseX() - (elementPreRender.x + 10 + gridCenter),-192, 192);
			dtpSource.y = CLAMP(-mouseOffset.y + GetMouseY() - (elementPreRender.y + 10 + gridCenter), -192, 192);
			break;
			case HeldOrigin :
			dtpOrigin.x = CLAMP(-mouseOffset.x - GetMouseX() + (elementRender.x + 10 + gridCenter), -192, 192);
			dtpOrigin.y = CLAMP(-mouseOffset.y - GetMouseY() + (elementRender.y + 10 + gridCenter), -192, 192);
			break;
			case HeldDest :
			dtpDest.x = CLAMP(-mouseOffset.x + GetMouseX() - (elementRender.x + 10 + gridCenter),-192, 192);
			dtpDest.y = CLAMP(-mouseOffset.y + GetMouseY() - (elementRender.y + 10 + gridCenter), -192, 192);
			break;
			case Released :
			// shouldn't be possible to reach here
			break;
		}
	}

	else if (IsMouseButtonReleased(0)) {
		dragState = Released;
		GuiUnlock();
	}
	//if mouse inside square 1, origin, square 2, square 3
	//	set state to move
	//if move state
	//	if inside
	//	move shape
	//if mouse let go: reset state
}

void DrawUI() {
	//elementSlider.x + elementBorderWidth
	char buffer[10];
	int xOffset = 60;
	int yOffset = 40;
	const int sliderSpacing = 10;
	const int sliderHeight = 20;
	const int sliderWidth = 200;

	GuiLabel(
			(Rectangle) {
			.x = elementSliders.x + xOffset - 25,
			.y = elementSliders.y + yOffset - sliderHeight - (sliderSpacing / 2),
			.width = sliderWidth,
			.height = sliderHeight
			},
			"Source"
			);

	sprintf(buffer, "%d", (int)dtpSource.x),
		GuiSlider(
				(Rectangle) {
				.x = elementSliders.x + xOffset,
				.y = elementSliders.y + yOffset,
				.width = sliderWidth,
				.height = sliderHeight
				},
				"x",
				buffer,
				&dtpSource.x,
				-192,
				192
				);
	sprintf(buffer, "%d", (int)dtpSource.y),
		GuiSlider(
				(Rectangle) {
				.x = elementSliders.x + xOffset,
				.y = elementSliders.y + yOffset + sliderHeight + sliderSpacing,
				.width = sliderWidth,
				.height = sliderHeight
				},
				"y",
				buffer,
				&dtpSource.y,
				-192,
				192
				);
	sprintf(buffer, "%d", (int)dtpSource.width),
		GuiSlider(
				(Rectangle) {
				.x = elementSliders.x + xOffset,
				.y = elementSliders.y + yOffset + ((sliderHeight + sliderSpacing) * 2),
				.width = sliderWidth,
				.height = sliderHeight
				},
				"Width",
				buffer,
				&dtpSource.width,
				-192,
				192
				);
	sprintf(buffer, "%d", (int)dtpSource.height),
		GuiSlider(
				(Rectangle) {
				.x = elementSliders.x + xOffset,
				.y = elementSliders.y + yOffset + ((sliderHeight + sliderSpacing) * 3),
				.width = sliderWidth,
				.height = sliderHeight
				},
				"Height",
				buffer,
				&dtpSource.height,
				-192,
				192
				);


	xOffset = 150 + sliderWidth;
	GuiLabel(
			(Rectangle) {
			.x = elementSliders.x + xOffset - 25,
			.y = elementSliders.y + yOffset - sliderHeight - (sliderSpacing / 2),
			.width = sliderWidth,
			.height = sliderHeight
			},
			"Dest"
			);
	sprintf(buffer, "%d", (int)dtpDest.x),
		GuiSlider(
				(Rectangle) {
				.x = elementSliders.x + xOffset,
				.y = elementSliders.y + yOffset,
				.width = sliderWidth,
				.height = sliderHeight
				},
				"x",
				buffer,
				&dtpDest.x,
				-192,
				192
				);
	sprintf(buffer, "%d", (int)dtpDest.y),
		GuiSlider(
				(Rectangle) {
				.x = elementSliders.x + xOffset,
				.y = elementSliders.y + yOffset + sliderHeight + sliderSpacing,
				.width = sliderWidth,
				.height = sliderHeight
				},
				"y",
				buffer,
				&dtpDest.y,
				-192,
				192
				);
	sprintf(buffer, "%d", (int)dtpDest.width),
		GuiSlider(
				(Rectangle) {
				.x = elementSliders.x + xOffset,
				.y = elementSliders.y + yOffset + ((sliderHeight + sliderSpacing) * 2),
				.width = sliderWidth,
				.height = sliderHeight
				},
				"Width",
				buffer,
				&dtpDest.width,
				-192,
				192
				);
	sprintf(buffer, "%d", (int)dtpDest.height),
		GuiSlider(
				(Rectangle) {
				.x = elementSliders.x + xOffset,
				.y = elementSliders.y + yOffset + ((sliderHeight + sliderSpacing) * 3),
				.width = sliderWidth,
				.height = sliderHeight
				},
				"Height",
				buffer,
				&dtpDest.height,
				-192,
				192
				);

	xOffset = 425 + sliderWidth;
	GuiLabel(
			(Rectangle) {
			.x = elementSliders.x + xOffset - 25,
			.y = elementSliders.y + yOffset - sliderHeight - (sliderSpacing / 2),
			.width = sliderWidth,
			.height = sliderHeight
			},
			"Origin"
			);
	sprintf(buffer, "%d", (int)dtpOrigin.x),
		GuiSlider(
				(Rectangle) {
				.x = elementSliders.x + xOffset,
				.y = elementSliders.y + yOffset,
				.width = sliderWidth,
				.height = sliderHeight
				},
				"x",
				buffer,
				&dtpOrigin.x,
				-192,
				192
				);
	sprintf(buffer, "%d", (int)dtpOrigin.y),
		GuiSlider(
				(Rectangle) {
				.x = elementSliders.x + xOffset,
				.y = elementSliders.y + yOffset + sliderHeight + sliderSpacing,
				.width = sliderWidth,
				.height = sliderHeight
				},
				"y",
				buffer,
				&dtpOrigin.y,
				-192,
				192
				);
	

	yOffset = 40 + 15 + elementSliders.y + sliderHeight + sliderSpacing,
			GuiLabel(
					(Rectangle) {
					.x = elementSliders.x + xOffset - 25,
					.y = elementSliders.y + yOffset - sliderHeight - (sliderSpacing / 2),
					.width = sliderWidth,
					.height = sliderHeight
					},
					"Rotation"
					);
	sprintf(buffer, "%d", (int)dtpRotation),
		GuiSlider(
				(Rectangle) {
				.x = elementSliders.x + xOffset,
				.y = elementSliders.y + yOffset,
				.width = sliderWidth,
				.height = sliderHeight
				},
				"",
				buffer,
				&dtpRotation,
				-360,
				360
				);
};

void CheckDifference() {
	if (predtpSource.x != dtpSource.x) {
		codePreviewHighlight[3].a = 255;
	}
	if (predtpSource.y != dtpSource.y) {
		codePreviewHighlight[4].a = 255;
	}
	if (predtpSource.width != dtpSource.width) {
		codePreviewHighlight[5].a = 255;
	}
	if (predtpSource.height != dtpSource.height) {
		codePreviewHighlight[6].a = 255;
	}
	if (predtpDest.x != dtpDest.x) {
		codePreviewHighlight[9].a = 255;
	}
	if (predtpDest.y != dtpDest.y) {
		codePreviewHighlight[10].a = 255;
	}
	if (predtpDest.width != dtpDest.width) {
		codePreviewHighlight[11].a = 255;
	}
	if (predtpDest.height != dtpDest.height) {
		codePreviewHighlight[12].a = 255;
	}
	if (predtpOrigin.x != dtpOrigin.x) {
		codePreviewHighlight[15].a = 255;
	}
	if (predtpOrigin.y != dtpOrigin.y) {
		codePreviewHighlight[16].a = 255;
	}
	if (predtpRotation != dtpRotation) {
		codePreviewHighlight[18].a = 255;
	}
}

void DrawCodeDisplay() {
	sprintf(codePreviewArray[3], "						.x = %d,", (int)dtpSource.x);
	sprintf(codePreviewArray[4], "						.y = %d,", (int)dtpSource.y);
	sprintf(codePreviewArray[5], "						.width = %d,", (int)dtpSource.width);
	sprintf(codePreviewArray[6], "						.height = %d", (int)dtpSource.height);
	sprintf(codePreviewArray[9], "						.x = %d,", (int)dtpDest.x);
	sprintf(codePreviewArray[10], "						.y = %d,", (int)dtpDest.y);
	sprintf(codePreviewArray[11], "						.width = %d,", (int)dtpDest.width);
	sprintf(codePreviewArray[12], "						.height = %d", (int)dtpDest.height);
	sprintf(codePreviewArray[15], "						.x = %d,", (int)dtpOrigin.x);
	sprintf(codePreviewArray[16], "						.y = %d,", (int)dtpOrigin.y);
	sprintf(codePreviewArray[18], "			%d, // Rotation", (int)dtpRotation);

	for(i = 0; i < 21; i += 1) {
		if (codePreviewHighlight[i].a > 0) {
			codePreviewHighlight[i].a -= 5;
		}
	}

	for(int b = 0; b < 21; b += 1) {
		Color tempColor = BLACK;
		if((b > 1) && (b < 8)) { tempColor = RED; }
		else if((b > 7) && (b < 14)) { tempColor = DARKGREEN; }
		else if((b > 13) && (b < 18)) { tempColor = BLUE; }
		else if(b == 18) { tempColor = DARKPURPLE; }
		DrawRectangle(
				0 + 20,
				elementCode.y + 15 + ((fontSize + 1) * b) - 1,
				previewElementResult.texture.width,
				fontSize+1,
				codePreviewHighlight[b]
				);
		DrawTextEx(
				sourceCodeFont,
				codePreviewArray[b],
				(Vector2) {
				.x = elementCode.x + 15,
				.y = elementCode.y + 15 + ((fontSize + 1) * b)
				},
				fontSize,
				fontSpacing,
				tempColor
				);
	}
};

void DrawOutput() {
	BeginTextureMode(previewElementPre);
	ClearBackground(RAYWHITE);
	int a;
	for( a = 0; a <= (int)previewElementPre.texture.width; a += gridSize){
		int lineWidth = 1;
		if (a == gridCenter) { lineWidth += 2; };
		DrawLineEx(
				(Vector2) {
				.x = a,
				.y = 0
				},
				(Vector2) {
				.x = a,
				.y = (int)previewElementPre.texture.height
				},
				lineWidth,
				BLACK
				);
	}
	for( a = 0; a <= (int)previewElementPre.texture.height; a += gridSize){
		int lineWidth = 1;
		if (a == gridCenter) { lineWidth += 2; };
		DrawLineEx(
				(Vector2) {
				.x = 0,
				.y = a
				},
				(Vector2) {
				.x = (int)previewElementPre.texture.width,
				.y = a
				},
				lineWidth,
				BLACK
				);
	}
	DrawTexturePro(
			sampleSprite,
			(Rectangle) {
			.x = 0,
			.y = 0,
			.width = sampleSprite.width,
			.height = sampleSprite.height,
			},
			(Rectangle) {
			.x = 0 + gridCenter,
			.y = 0 + gridCenter,
			.width = sampleSprite.width,
			.height = sampleSprite.height,
			},
			(Vector2){
			.x = 0,
			.y = 0
			},
			0,
			WHITE
			);
	Rectangle sourceRect = {
		.x = dtpSource.x + gridCenter,
		.y = dtpSource.y + gridCenter,
		.width = dtpSource.width,
		.height = dtpSource.height
	};
	if (sourceRect.width < 0) {
		sourceRect.width *= -1;
	}
	if (sourceRect.height < 0) {
		sourceRect.height *= -1;
	}
	DrawRectangleLinesEx(
			(Rectangle) {
			.x = sourceRect.x,
			.y = sourceRect.y,
			.width = sourceRect.width,
			.height = sourceRect.height,
			},
			2,
			RED
			);
	DrawTextEx(
			sourceCodeFont,
			"Source",
			(Vector2) {
			.x = sourceRect.x,
			.y = sourceRect.y - fontSize + 1
			},
			fontSize,
			fontSpacing,
			RED
			);
	EndTextureMode();
	DrawTexturePro(
			previewElementPre.texture,
			(Rectangle) {
			.x = 0,
			.y = 0,
			.width = previewElementPre.texture.width,
			.height = -previewElementPre.texture.height
			},
			(Rectangle) {
			.x = elementPreRender.x + 10,
			.y = elementPreRender.y + 10,
			.width = previewElementPre.texture.width,
			.height = previewElementPre.texture.height
			},
			(Vector2) {
			.x = 0,
			.y = 0
			},
			0,
			WHITE
			);
	BeginTextureMode(previewElementResult);
	ClearBackground(RAYWHITE);
	for( a = 0; a <= (int)previewElementResult.texture.width; a += gridSize){
		int lineWidth = 1;
		if (a == gridCenter) { lineWidth += 2; };
		DrawLineEx(
				(Vector2) {
				.x = a,
				.y = 0
				},
				(Vector2) {
				.x = a,
				.y = (int)previewElementResult.texture.height
				},
				lineWidth,
				BLACK
				);
	}
	for( a = 0; a <= (int)previewElementResult.texture.height; a += gridSize){
		int lineWidth = 1;
		if (a == gridCenter) { lineWidth += 2; };
		DrawLineEx(
				(Vector2) {
				.x = 0,
				.y = a
				},
				(Vector2) {
				.x = (int)previewElementResult.texture.width,
				.y = a
				},
				lineWidth,
				BLACK
				);
	}
	DrawTexturePro(
			sampleSprite,
			(Rectangle) {
			.x = dtpSource.x,
			.y = dtpSource.y,
			.width = dtpSource.width,
			.height = dtpSource.height,
			},
			(Rectangle) {
			.x = dtpDest.x + gridCenter,
			.y = dtpDest.y + gridCenter,
			.width = dtpDest.width,
			.height = dtpDest.height,
			},
			(Vector2){
			.x = dtpOrigin.x,
			.y = dtpOrigin.y
			},
			dtpRotation,
			WHITE
			);
	Rectangle destRect = {
		.x = dtpDest.x + gridCenter,
		.y = dtpDest.y + gridCenter,
		.width = dtpDest.width,
		.height = dtpDest.height
	};
	if (destRect.width < 0) {
		destRect.width *= -1;
	}
	if (destRect.height < 0) {
		destRect.height *= -1;

	}
	DrawRectangleLinesEx(
			(Rectangle) {
			.x = destRect.x - dtpOrigin.x,
			.y = destRect.y - dtpOrigin.y,
			.width = destRect.width,
			.height = destRect.height,
			},
			1,
			BLUE
			);
	DrawRectangleLinesEx(
			(Rectangle) {
			.x = destRect.x,
			.y = destRect.y,
			.width = destRect.width,
			.height = destRect.height,
			},
			2,
			DARKGREEN
			);
	DrawCircle(
			destRect.x - dtpOrigin.x,
			destRect.y - dtpOrigin.y,
			5,
			BLUE
			);
	DrawTextEx(
			sourceCodeFont,
			"Origin",
			(Vector2) {
			.x = destRect.x - 48 - dtpOrigin.x,
			.y = destRect.y - fontSize + 16 - dtpOrigin.y
			},
			fontSize,
			fontSpacing,
			BLUE
			);
	DrawTextEx(
			sourceCodeFont,
			"Dest",
			(Vector2) {
			.x = destRect.x + 8,
			.y = destRect.y - fontSize + 1
			},
			fontSize,
			fontSpacing,
			DARKGREEN
			);
	EndTextureMode();
	DrawTexturePro(
			previewElementResult.texture,
			(Rectangle) {
			.x = 0,
			.y = 0,
			.width = previewElementResult.texture.width,
			.height = -previewElementResult.texture.height
			},
			(Rectangle) {
			.x = elementRender.x + 10,
			.y = elementRender.y + 10,
			.width = previewElementResult.texture.width,
			.height = previewElementResult.texture.height
			},
			(Vector2) {
			.x = 0,
			.y = 0
			},
			0,
			WHITE
			);

};

void HandleDroppedFiles()
{
	if (IsFileDropped()) {
		FilePathList droppedFiles = LoadDroppedFiles();
		Texture2D tmp = LoadTexture(droppedFiles.paths[0]);
		if (tmp.id != 0) {
			// Texture loaded successfully
			UnloadTexture(sampleSprite);
			sampleSprite = tmp;
		}
		UnloadDroppedFiles(droppedFiles);
	}
}
