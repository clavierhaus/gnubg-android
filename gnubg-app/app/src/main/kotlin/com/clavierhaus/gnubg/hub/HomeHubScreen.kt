package com.clavierhaus.gnubg.hub

import androidx.compose.foundation.Image
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.text.BasicText
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Shadow
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.clavierhaus.gnubg.R

@Composable
fun HomeHubScreen(
    onPlay: () -> Unit,
    onLearn: () -> Unit,
    onAnalyse: () -> Unit,
    onOptions: () -> Unit,
    onProfile: () -> Unit
) {
    Box(modifier = Modifier.fillMaxSize()) {
        Image(
            painter = painterResource(id = R.drawable.home_hub_background),
            contentDescription = null,
            modifier = Modifier.fillMaxSize(),
            contentScale = ContentScale.Crop
        )

        BasicText(
            text = "GNU Backgammon",
            style = HomeTitleStyle,
            modifier = Modifier
                .align(Alignment.TopStart)
                .padding(start = 48.dp, top = 38.dp)
        )

        Column(
            modifier = Modifier
                .align(Alignment.CenterStart)
                .offset(x = 64.dp)
        ) {
            HomeHubEntry("Play", onPlay)
            Spacer(modifier = Modifier.height(22.dp))
            HomeHubEntry("Learn", onLearn)
            Spacer(modifier = Modifier.height(22.dp))
            HomeHubEntry("Analyse", onAnalyse)
            Spacer(modifier = Modifier.height(22.dp))
            HomeHubEntry("Options", onOptions)
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

private val HomeTitleStyle = TextStyle(
    color = Color.White,
    fontSize = 34.sp,
    fontWeight = FontWeight.SemiBold,
    shadow = Shadow(
        color = Color.Black,
        offset = Offset(2f, 2f),
        blurRadius = 8f
    )
)

private val HomeEntryStyle = TextStyle(
    color = Color.White,
    fontSize = 36.sp,
    fontWeight = FontWeight.Medium,
    shadow = Shadow(
        color = Color.Black,
        offset = Offset(2f, 2f),
        blurRadius = 8f
    )
)

private val HomeSecondaryStyle = TextStyle(
    color = Color.White,
    fontSize = 26.sp,
    fontWeight = FontWeight.Medium,
    shadow = Shadow(
        color = Color.Black,
        offset = Offset(2f, 2f),
        blurRadius = 8f
    )
)
