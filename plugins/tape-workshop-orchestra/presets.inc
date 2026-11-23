// SPDX-License-Identifier: AGPL-3.0-only
// Included only inside the shared sampler's preset() function.
n=std::clamp(n,0,programCount-1);
for(int i=0;i<Count;++i)values[i]=defaults[i];
values[Violin]=.8f;values[Viola]=.42f;values[Cello]=.38f;values[Bass]=0;
values[Reverb]=.28f;values[Attack]=.14f;values[Dynamics]=.65f;
if(n>=4&&n<=23)for(int i=Violin;i<=Bass;++i)values[i]=0;
auto section=[&](unsigned family,unsigned player){values[Violin+family]=1;values[StringPlayer+family]=player/float(melodee::orchestra_count[family]-1);};
switch(n){
    case 1:values[Viola]=.2f;values[Cello]=.2f;values[Reverb]=.15f;values[Dynamics]=.45f;break;
    case 2:values[Dynamics]=.15f;values[Cutoff]=.62f;values[Attack]=.25f;break;
    case 3:values[Attack]=.65f;values[Release]=.6f;values[MotionRate]=.08f;values[MotionDepth]=.35f;break;
    case 4:section(0,0);values[Ensemble]=1;values[Width]=1;values[Reverb]=.4f;break;
    case 5:section(0,1);values[Reverb]=.12f;break;
    case 6:section(0,2);values[Cutoff]=.7f;break;
    case 7:section(0,3);break;
    case 8:section(0,4);break;
    case 9:section(1,0);break;
    case 10:section(1,1);values[Dynamics]=.9f;break;
    case 11:section(1,2);break;
    case 12:section(1,3);break;
    case 13:section(2,0);break;
    case 14:section(2,1);values[Vibrato]=.1f;break;
    case 15:section(2,2);values[Cutoff]=.7f;break;
    case 16:section(2,3);break;
    case 17:section(3,0);break;
    case 18:section(3,1);break;
    case 19:section(3,2);values[Reverb]=.4f;break;
    case 20:section(3,3);values[Reverb]=.4f;break;
    case 21:section(0,0);values[Articulation]=1;break;
    case 22:section(1,1);values[Articulation]=1;break;
    case 23:section(2,0);values[Articulation]=1;break;
    case 24:values[Articulation]=1;values[Dynamics]=.9f;break;
    case 25:for(int i=Violin;i<=Bass;++i)values[i]=0;section(0,1);values[Legato]=1;values[GlideTime]=.12f;break;
    case 26:for(int i=Violin;i<=Bass;++i)values[i]=0;section(1,0);values[Legato]=1;values[GlideTime]=.1f;break;
    case 27:for(int i=Violin;i<=Bass;++i)values[i]=0;section(2,0);values[Legato]=1;values[GlideTime]=.08f;break;
    case 28:values[MotionDepth]=.6f;values[MotionRate]=.3f;values[MotionTarget]=1.f/3;break;
    case 29:values[RoomEcho]=.3f;values[EchoTime]=.4f;values[EchoFeedback]=.5f;break;
    case 30:values[Cutoff]=.35f;values[Dynamics]=.2f;values[Attack]=.4f;values[Release]=.65f;break;
    case 31:values[Violin]=.8f;values[Viola]=.7f;values[Cello]=.55f;values[Dynamics]=1;values[Reverb]=.4f;break;
}
if(n>=17&&n<=24){values[Attack]=0;values[Release]=.12f;}
program=n;
