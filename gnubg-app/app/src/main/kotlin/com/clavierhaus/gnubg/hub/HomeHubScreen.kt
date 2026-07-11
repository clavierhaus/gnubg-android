package com.clavierhaus.gnubg.hub

import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.Icon
import androidx.compose.foundation.text.BasicText
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.blur
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Shadow
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.font.Font
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.withStyle
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.clavierhaus.gnubg.R

@Composable
fun HomeHubScreen(
    onPlay: () -> Unit,
    onCoach: () -> Unit,
    onAnalysePosition: () -> Unit,
    onReviewMatch: () -> Unit,
    onOptions: () -> Unit,
    onProfile: () -> Unit
) {
    Box(modifier = Modifier.fillMaxSize().background(Color.Black)) {
        Image(
            painter = painterResource(id = R.drawable.home_hub_background),
            contentDescription = null,
            // Fit preserves aspect (no distortion) and is device-independent;
            // wider screens get black bars left/right, taller ones top/bottom.
            modifier = Modifier.fillMaxSize(),
            contentScale = ContentScale.Fit
        )

        // The same gear as in-game (Icons.Filled.Settings), wearing the hub's own
        // treatment: white, over the same drop shadow the text carries.
        //
        // TextStyle.Shadow is not available to an Icon, so it is reproduced: a black
        // copy behind, displaced and blurred by the same amounts. Those amounts are
        // PIXELS in Shadow -- Offset(2f, 2f) and blurRadius = 8f -- so they are
        // converted through the density rather than guessed at in dp.
        val density = LocalDensity.current
        val shadowShift = with(density) { HOME_SHADOW_OFFSET_PX.toDp() }
        val shadowBlur  = with(density) { HOME_SHADOW_BLUR_PX.toDp() }

        Box(
            modifier = Modifier
                .align(Alignment.TopStart)
                // Room for the blur to spread without being clipped by the Box.
                .padding(start = 48.dp - shadowBlur, top = 18.dp - shadowBlur)
                .padding(shadowBlur)
        ) {
            Icon(
                imageVector = Icons.Filled.Settings,
                contentDescription = null,
                tint = Color.Black,
                modifier = Modifier
                    .offset(x = shadowShift, y = shadowShift)
                    .blur(shadowBlur)
                    .size(GEAR_SIZE)
            )
            Icon(
                imageVector = Icons.Filled.Settings,
                contentDescription = "Options",
                tint = GnuWhite,
                modifier = Modifier
                    .size(GEAR_SIZE)
                    .clickable(onClick = onOptions)
            )
        }

        // "GNU" orange, "Backgammon" off-white, DejaVu Serif -- the colours and face
        // of ic_launcher_foreground.png, sampled from it rather than approximated:
        // #F5A623 and #F5F5F5.
        BasicText(
            text = buildAnnotatedString {
                withStyle(SpanStyle(color = GnuOrange)) { append("GNU") }
                append(" ")
                withStyle(SpanStyle(color = GnuWhite)) { append("Backgammon") }
            },
            style = HomeTitleStyle,
            modifier = Modifier
                .align(Alignment.TopStart)
                .padding(start = 48.dp, top = 66.dp)
        )

        // padding, not offset. An offset composable keeps the layout slot its parent
        // allocated; if it is pushed outside the parent's bounds it stops receiving
        // pointer events, which is how the match-setup screen once grew a button that
        // could be seen but not tapped. padding moves the slot itself, so the question
        // does not arise.
        // The column is centre-aligned, so top padding p lowers its content by p/2.
        // 32dp of padding buys 16dp of clearance from the title. The 22dp spacers
        // between the entries are untouched.
        Column(
            modifier = Modifier
                .align(Alignment.CenterStart)
                .padding(start = 64.dp, top = 32.dp)
        ) {
            HomeHubEntry("Play Tournament Match", onPlay)
            Spacer(modifier = Modifier.height(22.dp))
            // The fourth mode (docs/COACH.md): play gnubg with the engine
            // looking over your shoulder. Second position: learning sits
            // between competing and analysing.
            HomeHubEntry("Train with the Coach", onCoach)
            Spacer(modifier = Modifier.height(22.dp))
            // Second: the feature people still open XG Mobile for.
            HomeHubEntry("Analyse Position", onAnalysePosition)
            Spacer(modifier = Modifier.height(22.dp))
            // Third, now that it exists.
            HomeHubEntry("Review Match", onReviewMatch)
        }

        BasicText(
            text = "Profile",
            style = HomeSecondaryStyle,
            modifier = Modifier
                .align(Alignment.BottomEnd)
                .clickable(onClick = onProfile)
                .padding(end = 42.dp, bottom = 34.dp)
        )
    }
}

@Composable
private fun HomeHubEntry(
    label: String,
    onClick: () -> Unit
) {
    BasicText(
        text = label,
        style = HomeEntryStyle,
        modifier = Modifier
            .clickable(onClick = onClick)
            .padding(vertical = 4.dp)
    )
}

// Sampled from ic_launcher_foreground.png. The icon is the app's signature; the
// hub should not invent a second one.
private val GnuOrange = Color(0xFFF5A623)
private val GnuWhite  = Color(0xFFF5F5F5)

// DejaVu Serif, vendored in res/font. Bitstream Vera derived, free to redistribute.
private val DejaVuSerif = FontFamily(
    Font(R.font.dejavu_serif, FontWeight.Normal),
    Font(R.font.dejavu_serif_bold, FontWeight.Bold)
)

// The hub's drop shadow, in the units Shadow uses: pixels.
private const val HOME_SHADOW_OFFSET_PX = 2f
private const val HOME_SHADOW_BLUR_PX = 8f
private val GEAR_SIZE = 36.dp

private val HomeShadow = Shadow(
    color = Color.Black,
    offset = Offset(HOME_SHADOW_OFFSET_PX, HOME_SHADOW_OFFSET_PX),
    blurRadius = HOME_SHADOW_BLUR_PX
)

private val HomeTitleStyle = TextStyle(
    fontFamily = DejaVuSerif,
    fontSize = 34.sp,
    fontWeight = FontWeight.Bold,
    shadow = HomeShadow
)

private val HomeEntryStyle = TextStyle(
    fontFamily = DejaVuSerif,
    color = GnuWhite,
    fontSize = 36.sp,
    fontWeight = FontWeight.Normal,
    shadow = HomeShadow
)

private val HomeSecondaryStyle = TextStyle(
    fontFamily = DejaVuSerif,
    color = GnuWhite,
    fontSize = 26.sp,
    fontWeight = FontWeight.Normal,
    shadow = HomeShadow
)
