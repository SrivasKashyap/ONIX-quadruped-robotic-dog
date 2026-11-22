import { NextResponse } from 'next/server'
import { supabase } from '../../../../lib/supabase'

export async function POST(request: Request) {
  try {
    const body = await request.json()
    const { lat, lng, speed, battery } = body

    if (lat === undefined || lng === undefined) {
      return NextResponse.json({ error: 'Missing lat/lng payload' }, { status: 400 })
    }

    const { data, error } = await supabase
      .from('dog_status')
      .upsert([
        {
          id: 1,
          lat,
          lng,
          speed: speed || 0,
          battery: battery || 100,
          status: 'running',
          updated_at: new Date().toISOString()
        }
      ])
      .select()

    if (error) {
      return NextResponse.json({ error: error.message }, { status: 500 })
    }

    return NextResponse.json({ success: true, telemetry: data }, { status: 200 })
  } catch (err: any) {
    return NextResponse.json({ error: err.message || 'Internal Server Error' }, { status: 500 })
  }
}