using System;
using System.Collections.Generic;
using CocosSharp;
using Microsoft.Xna.Framework;

namespace CocosSharpGame2
{
    public class IntroLayer : CCLayerColor
    {

        // Define a label variable
        CCLabel label; CCLabel label1;
        CCSprite sp;
        public IntroLayer() : base(CCColor4B.Blue)
        {

            // create and initialize a Label
            label = new CCLabel("Hello CocosSharp", "fonts/MarkerFelt", 22, CCLabelFormat.SpriteFont);

            // add the label as a child to this Layer
            AddChild(label);

            label1 = new CCLabel("Hello Amit.. wie gehts", "fonts/MarkerFelt", 22, CCLabelFormat.SpriteFont);
            label1.PositionX = 200;
            label1.PositionY = 200;
            AddChild(label1);

            sp = new CCSprite("GameThumbnail");
            sp.PositionX = 200;
            sp.PositionY = 200;
            AddChild(sp);

        }

        protected override void AddedToScene()
        {
            base.AddedToScene();

            // Use the bounds to layout the positioning of our drawable assets
            var bounds = VisibleBoundsWorldspace;

            // position the label on the center of the screen
            label.Position = bounds.Center;

            // Register for touch events
            var touchListener = new CCEventListenerTouchAllAtOnce();
            touchListener.OnTouchesEnded = OnTouchesEnded;
            AddEventListener(touchListener, this);
        }

        void OnTouchesEnded(List<CCTouch> touches, CCEvent touchEvent)
        {
            if (touches.Count > 0)
            {
                // Perform touch handling here
            }
        }
    }
}

