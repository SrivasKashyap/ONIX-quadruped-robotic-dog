import { NextResponse } from 'next/server'
import { supabase } from '../../../../lib/supabase'

export async function POST(request: Request) {
  try {
    const body = await request.json()
    const { lat, lng, place_name, image_url, trip_id } = body

    if (!lat || !lng) {
      return NextResponse.json({ error: 'Missing lat or lng coordinates' }, { status: 400 })
    }

    const { data, error } = await supabase
      .from('defects')
      .insert([
        {
          lat,
          lng,
          place_name: place_name || 'Unidentified Track Segment',
          image_url: image_url || 'https://via.placeholder.com/150',
          trip_id: trip_id || 1,
          detected_at: new Date().toISOString()
        }
      ])
      .select()

    if (error) {
      return NextResponse.json({ error: error.message }, { status: 500 })
    }

    return NextResponse.json({ success: true, defect: data }, { status: 201 })
  } catch (err: any) {
    return NextResponse.json({ error: err.message || 'Internal Server Error' }, { status: 500 })
  }
}